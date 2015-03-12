//
// interrupt.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2015  R. Stange <rsta2@o2online.de>
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
#include <circle/bcm2835.h>
#include <circle/memio.h>
#include <circle/types.h>
#include <assert.h>

#define ARM_IC_IRQ_PENDING(irq)	(  (irq) < ARM_IRQ2_BASE	\
				 ? ARM_IC_IRQ_PENDING_1		\
				 : ((irq) < ARM_IRQBASIC_BASE	\
				   ? ARM_IC_IRQ_PENDING_2	\
				   : ARM_IC_IRQ_BASIC_PENDING))
#define ARM_IC_IRQS_ENABLE(irq)	(  (irq) < ARM_IRQ2_BASE	\
				 ? ARM_IC_ENABLE_IRQS_1		\
				 : ((irq) < ARM_IRQBASIC_BASE	\
				   ? ARM_IC_ENABLE_IRQS_2	\
				   : ARM_IC_ENABLE_BASIC_IRQS))
#define ARM_IC_IRQS_DISABLE(irq) (  (irq) < ARM_IRQ2_BASE	\
				 ? ARM_IC_DISABLE_IRQS_1	\
				 : ((irq) < ARM_IRQBASIC_BASE	\
				   ? ARM_IC_DISABLE_IRQS_2	\
				   : ARM_IC_DISABLE_BASIC_IRQS))
#define ARM_IRQ_MASK(irq)	(1 << ((irq) & (ARM_IRQS_PER_REG-1)))
				   
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

CInterruptSystem::~CInterruptSystem ()
{
	s_pThis = 0;
}

boolean CInterruptSystem::Initialize (void)
{
	TExceptionTable *pTable = (TExceptionTable *) ARM_EXCEPTION_TABLE_BASE;
	pTable->IRQ = ARM_OPCODE_BRANCH (ARM_DISTANCE (pTable->IRQ, IRQStub));

	CleanDataCache ();
	DataSyncBarrier ();

	InvalidateInstructionCache ();
	FlushBranchTargetCache ();
	DataSyncBarrier ();

	InstructionSyncBarrier ();

	DataMemBarrier ();

	write32 (ARM_IC_FIQ_CONTROL, 0);

	write32 (ARM_IC_DISABLE_IRQS_1, (u32) -1);
	write32 (ARM_IC_DISABLE_IRQS_2, (u32) -1);
	write32 (ARM_IC_DISABLE_BASIC_IRQS, (u32) -1);

	// Ack pending IRQs
	write32 (ARM_IC_IRQ_BASIC_PENDING, read32 (ARM_IC_IRQ_BASIC_PENDING));
	write32 (ARM_IC_IRQ_PENDING_1, 	   read32 (ARM_IC_IRQ_PENDING_1));
	write32 (ARM_IC_IRQ_PENDING_2,     read32 (ARM_IC_IRQ_PENDING_2));

	DataMemBarrier ();

	EnableInterrupts ();

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

void CInterruptSystem::EnableIRQ (unsigned nIRQ)
{
	DataMemBarrier ();

	assert (nIRQ < IRQ_LINES);

	write32 (ARM_IC_IRQS_ENABLE (nIRQ), ARM_IRQ_MASK (nIRQ));

	DataMemBarrier ();
}

void CInterruptSystem::DisableIRQ (unsigned nIRQ)
{
	DataMemBarrier ();

	assert (nIRQ < IRQ_LINES);

	write32 (ARM_IC_IRQS_DISABLE (nIRQ), ARM_IRQ_MASK (nIRQ));

	DataMemBarrier ();
}

CInterruptSystem *CInterruptSystem::Get (void)
{
	assert (s_pThis != 0);
	return s_pThis;
}

int CInterruptSystem::CallIRQHandler (unsigned nIRQ)
{
	assert (nIRQ < IRQ_LINES);
	TIRQHandler *pHandler = m_apIRQHandler[nIRQ];

	if (pHandler != 0)
	{
		(*pHandler) (m_pParam[nIRQ]);
		
		return 1;
	}
	else
	{
		DisableIRQ (nIRQ);
	}
	
	return 0;
}

void CInterruptSystem::InterruptHandler (void)
{
	assert (s_pThis != 0);

#ifdef ARM_ALLOW_MULTI_CORE
	if (CMultiCoreSupport::LocalInterruptHandler ())
	{
		return;
	}
#endif

	for (unsigned nIRQ = 0; nIRQ < IRQ_LINES; nIRQ++)
	{
		u32 nPendReg = ARM_IC_IRQ_PENDING (nIRQ);
		u32 nIRQMask = ARM_IRQ_MASK (nIRQ);
		
		if (read32 (nPendReg) & nIRQMask)
		{
			if (s_pThis->CallIRQHandler (nIRQ))
			{
				write32 (nPendReg, nIRQMask);
			
				return;
			}

			write32 (nPendReg, nIRQMask);
		}
	}
}

void InterruptHandler (void)
{
	DataMemBarrier ();
	
	CInterruptSystem::InterruptHandler ();

	DataMemBarrier ();
}
