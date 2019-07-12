//
// interruptgic.cpp
//
// Driver for the GIC-400 interrupt controller of the Raspberry Pi 4
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2019  R. Stange <rsta2@o2online.de>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <circle/interrupt.h>
#include <circle/synchronize.h>
#include <circle/multicore.h>
#include <circle/bcm2711.h>
#include <circle/memio.h>
#include <circle/sysconfig.h>
#include <circle/types.h>
#include <assert.h>

// GIC distributor registers (non-secure access)
#define GICD_CTLR		(ARM_GICD_BASE + 0x000)
	#define GICD_CTLR_DISABLE	(0 << 0)
	#define GICD_CTLR_ENABLE	(1 << 0)
#define GICD_ISENABLER0		(ARM_GICD_BASE + 0x100)
#define GICD_ICENABLER0		(ARM_GICD_BASE + 0x180)
#define GICD_ISPENDR0		(ARM_GICD_BASE + 0x200)
#define GICD_ICPENDR0		(ARM_GICD_BASE + 0x280)
#define GICD_ISACTIVER0		(ARM_GICD_BASE + 0x300)
#define GICD_ICACTIVER0		(ARM_GICD_BASE + 0x380)
#define GICD_IPRIORITYR0	(ARM_GICD_BASE + 0x400)
	#define GICD_IPRIORITYR_DEFAULT	0xA0
#define GICD_ITARGETSR0		(ARM_GICD_BASE + 0x800)
	#define GICD_ITARGETSR_CORE0	(1 << 0)
#define GICD_ICFGR0		(ARM_GICD_BASE + 0xC00)
	#define GICD_ICFGR_LEVEL_SENSITIVE	(0 << 1)
	#define GICD_ICFGR_EDGE_TRIGGERED	(1 << 1)
#define GICD_SGIR		(ARM_GICD_BASE + 0xF00)
	#define GICD_SGIR_SGIINTID__MASK		0x0F
	#define GICD_SGIR_CPU_TARGET_LIST__SHIFT	16
	#define GICD_SGIR_TARGET_LIST_FILTER__SHIFT	24

// GIC CPU interface registers (non-secure access)
#define GICC_CTLR		(ARM_GICC_BASE + 0x000)
	#define GICC_CTLR_DISABLE	(0 << 0)
	#define GICC_CTLR_ENABLE	(1 << 0)
#define GICC_PMR		(ARM_GICC_BASE + 0x004)
	#define GICC_PMR_PRIORITY	(0xF0 << 0)
#define GICC_IAR		(ARM_GICC_BASE + 0x00C)
	#define GICC_IAR_INTERRUPT_ID__MASK	0x3FF
	#define GICC_IAR_CPUID__SHIFT		10
	#define GICC_IAR_CPUID__MASK		(3 << 10)
#define GICC_EOIR		(ARM_GICC_BASE + 0x010)
	#define GICC_EOIR_EOIINTID__MASK	0x3FF
	#define GICC_EOIR_CPUID__SHIFT		10
	#define GICC_EOIR_CPUID__MASK		(3 << 10)

CInterruptSystem *CInterruptSystem::s_pThis = 0;

CInterruptSystem::CInterruptSystem (void)
{
	for (unsigned nIRQ = 0; nIRQ < IRQ_LINES; nIRQ++)
	{
		m_apIRQHandler[nIRQ] = 0;
		m_pParam[nIRQ] = 0;
	}

	s_pThis = this;
}

CInterruptSystem::~CInterruptSystem (void)
{
	DisableIRQs ();

	write32 (GICD_CTLR, GICD_CTLR_DISABLE);

	s_pThis = 0;
}

boolean CInterruptSystem::Initialize (void)
{
#if AARCH == 32
	TExceptionTable *pTable = (TExceptionTable *) ARM_EXCEPTION_TABLE_BASE;
	pTable->IRQ = ARM_OPCODE_BRANCH (ARM_DISTANCE (pTable->IRQ, IRQStub));
	pTable->FIQ = ARM_OPCODE_BRANCH (ARM_DISTANCE (pTable->FIQ, FIQStub));

	SyncDataAndInstructionCache ();
#endif

	// initialize distributor:

	write32 (GICD_CTLR, GICD_CTLR_DISABLE);

	// disable, acknowledge and deactivate all interrupts
	for (unsigned n = 0; n < IRQ_LINES/32; n++)
	{
		write32 (GICD_ICENABLER0 + 4*n, ~0);
		write32 (GICD_ICPENDR0   + 4*n, ~0);
		write32 (GICD_ICACTIVER0 + 4*n, ~0);
	}

	// direct all interrupts to core 0 with default priority
	for (unsigned n = 0; n < IRQ_LINES/4; n++)
	{
		write32 (GICD_IPRIORITYR0 + 4*n,   GICD_IPRIORITYR_DEFAULT
						 | GICD_IPRIORITYR_DEFAULT << 8
						 | GICD_IPRIORITYR_DEFAULT << 16
						 | GICD_IPRIORITYR_DEFAULT << 24);

		write32 (GICD_ITARGETSR0 + 4*n,   GICD_ITARGETSR_CORE0
						| GICD_ITARGETSR_CORE0 << 8
						| GICD_ITARGETSR_CORE0 << 16
						| GICD_ITARGETSR_CORE0 << 24);
	}

	// set all interrupts to level triggered
	for (unsigned n = 0; n < IRQ_LINES/16; n++)
	{
		write32 (GICD_ICFGR0 + 4*n, 0);
	}

	write32 (GICD_CTLR, GICD_CTLR_ENABLE);

	// initialize core 0 CPU interface:

	write32 (GICC_PMR, GICC_PMR_PRIORITY);
	write32 (GICC_CTLR, GICC_CTLR_ENABLE);

	EnableIRQs ();

	return TRUE;
}

void CInterruptSystem::ConnectIRQ (unsigned nIRQ, TIRQHandler *pHandler, void *pParam)
{
	assert (nIRQ < IRQ_LINES);
	assert (m_apIRQHandler[nIRQ] == 0);

	m_apIRQHandler[nIRQ] = pHandler;
	m_pParam[nIRQ] = pParam;

	EnableIRQ (nIRQ);
}

void CInterruptSystem::DisconnectIRQ (unsigned nIRQ)
{
	assert (nIRQ < IRQ_LINES);
	assert (m_apIRQHandler[nIRQ] != 0);

	DisableIRQ (nIRQ);

	m_apIRQHandler[nIRQ] = 0;
	m_pParam[nIRQ] = 0;
}

void CInterruptSystem::ConnectFIQ (unsigned nFIQ, TFIQHandler *pHandler, void *pParam)
{
	assert (nFIQ <= ARM_MAX_FIQ);
	assert (pHandler != 0);
	assert (FIQData.pHandler == 0);

	FIQData.pHandler = pHandler;
	FIQData.pParam = pParam;

	EnableFIQ (nFIQ);
}

void CInterruptSystem::DisconnectFIQ (void)
{
	assert (FIQData.pHandler != 0);

	DisableFIQ ();

	FIQData.pHandler = 0;
	FIQData.pParam = 0;
}

void CInterruptSystem::EnableIRQ (unsigned nIRQ)
{
	assert (nIRQ < IRQ_LINES);

	write32 (GICD_ISENABLER0 + 4 * (nIRQ / 32), 1 << (nIRQ % 32));
}

void CInterruptSystem::DisableIRQ (unsigned nIRQ)
{
	assert (nIRQ < IRQ_LINES);

	write32 (GICD_ICENABLER0 + 4 * (nIRQ / 32), 1 << (nIRQ % 32));
}

void CInterruptSystem::EnableFIQ (unsigned nFIQ)
{
	assert (0);	// TODO: support FIQ
}

void CInterruptSystem::DisableFIQ (void)	// may be called, when FIQ is not enabled
{
	// TODO: support FIQ
}

CInterruptSystem *CInterruptSystem::Get (void)
{
	assert (s_pThis != 0);
	return s_pThis;
}

boolean CInterruptSystem::CallIRQHandler (unsigned nIRQ)
{
	assert (nIRQ < IRQ_LINES);
	TIRQHandler *pHandler = m_apIRQHandler[nIRQ];

	if (pHandler != 0)
	{
		(*pHandler) (m_pParam[nIRQ]);
		
		return TRUE;
	}
	else
	{
		DisableIRQ (nIRQ);
	}
	
	return FALSE;
}

void CInterruptSystem::InterruptHandler (void)
{
	u32 nIAR = read32 (GICC_IAR);

	unsigned nIRQ = nIAR & GICC_IAR_INTERRUPT_ID__MASK;
	if (nIRQ < IRQ_LINES)
	{
		if (nIRQ > 15)
		{
			// peripheral interrupts (PPI and SPI)
			assert (s_pThis != 0);
			s_pThis->CallIRQHandler (nIRQ);
		}
#ifdef ARM_ALLOW_MULTI_CORE
		else
		{
			// software generated interrupts (SGI)
			unsigned nFromCore = (nIAR & GICC_IAR_CPUID__MASK) >> GICC_IAR_CPUID__SHIFT;
			CMultiCoreSupport::LocalInterruptHandler (nFromCore, nIRQ);
		}
#endif

		write32 (GICC_EOIR, nIAR);
	}
#ifndef NDEBUG
	else
	{
		// spurious interrupts
		assert (nIRQ >= 1020);
	}
#endif
}

void InterruptHandler (void)
{
	CInterruptSystem::InterruptHandler ();
}

void CInterruptSystem::InitializeSecondary (void)
{
	// initialize CPU interface of secondary core
	write32 (GICC_PMR, GICC_PMR_PRIORITY);
	write32 (GICC_CTLR, GICC_CTLR_ENABLE);
}

void CInterruptSystem::SendIPI (unsigned nCore, unsigned nIPI)
{
	assert (nCore <= 7);
	assert (nIPI <= 15);

	// generate software interrupt (SGI)
	write32 (GICD_SGIR,   1 << (nCore + GICD_SGIR_CPU_TARGET_LIST__SHIFT)
			    | nIPI);
}
