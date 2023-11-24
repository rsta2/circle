//
// southbridge.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2023  R. Stange <rsta2@o2online.de>
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
#include <circle/southbridge.h>
#include <circle/memio.h>
#include <circle/bcm2712.h>
#include <circle/logger.h>
#include <circle/macros.h>
#include <assert.h>

#define RP1_PCIE_SLOT			0
#define RP1_PCIE_FUNC			0

#define RP1_PCI_CLASS_CODE		0x20000

// Second level interrupt controller
#define INTC_REG_RW			(ARM_RP1_INTC + 0x000)
#define INTC_INT_STAT_LOW		(ARM_RP1_INTC + 0x108)
#define INTC_INT_STAT_HIGH		(ARM_RP1_INTC + 0x10C)
#define INTC_REG_SET			(ARM_RP1_INTC + 0x800)
#define INTC_REG_CLR			(ARM_RP1_INTC + 0xC00)

#define MSIX_CFG(irq)			(0x008 + (irq)*4)
	#define MSIX_CFG_ENABLE		BIT (0)
	#define MSIX_CFG_TEST		BIT (1)		// forces interrupt
	#define MSIX_CFG_IACK		BIT (2)
	#define MSIX_CFG_IACK_ENABLE	BIT (3)

LOGMODULE ("rp1");

CSouthbridge *CSouthbridge::s_pThis = nullptr;

boolean CSouthbridge::s_bIsInitialized = FALSE;

CSouthbridge::CSouthbridge (CInterruptSystem *pInterrupt)
:	m_pInterrupt (pInterrupt),
	m_pPCIe (nullptr),
	m_ulEnableMask (0)
{
	if (s_pThis)
	{
		return;
	}
	s_pThis = this;

	for (unsigned nIRQ = 0; nIRQ < RP1_IRQ_LINES; nIRQ++)
	{
		m_apIRQHandler[nIRQ] = nullptr;
		m_pParam[nIRQ] = nullptr;
		m_bEdgeTriggered[nIRQ] = FALSE;
	}

	assert (m_pInterrupt);
	m_pPCIe = new CBcmPCIeHostBridge (m_pInterrupt);
}

CSouthbridge::~CSouthbridge (void)
{
	if (s_pThis != this)
	{
		return;
	}

	if (s_bIsInitialized)
	{
		assert (m_pInterrupt);
		m_pInterrupt->DisconnectIRQ (ARM_IRQ_PCIE_HOST_INTA);

		// TODO: Disable all enabled IRQs

		s_bIsInitialized = FALSE;
	}

	delete m_pPCIe;
	m_pPCIe = nullptr;

	s_pThis = nullptr;
}

boolean CSouthbridge::Initialize (void)
{
	if (s_pThis != this)
	{
		return s_bIsInitialized;
	}

	assert (!s_bIsInitialized);

	assert (m_pPCIe);
	if (!m_pPCIe->Initialize ())
	{
		LOGERR ("Cannot initialize PCIe host bridge");

		return FALSE;
	}

	if (!m_pPCIe->EnableDevice (RP1_PCI_CLASS_CODE, RP1_PCIE_SLOT, RP1_PCIE_FUNC))
	{
		LOGERR ("Cannot enable RP1 device");
	}

	s_bIsInitialized = TRUE;

	assert (m_pInterrupt);
	m_pInterrupt->ConnectIRQ (ARM_IRQ_PCIE_HOST_INTA, InterruptHandler, this);

	return TRUE;
}

void CSouthbridge::ConnectIRQ (unsigned nIRQ, TIRQHandler *pHandler, void *pParam)
{
	if (s_pThis != this)
	{
		s_pThis->ConnectIRQ (nIRQ, pHandler, pParam);

		return;
	}

	assert (s_bIsInitialized);

	assert (nIRQ & IRQ_FROM_RP1__MASK);
	unsigned nIRQLocal = nIRQ & IRQ_NUMBER__MASK;

	assert (nIRQLocal < RP1_IRQ_LINES);
	assert (!m_apIRQHandler[nIRQLocal]);

	m_apIRQHandler[nIRQLocal] = pHandler;
	m_pParam[nIRQLocal] = pParam;

	EnableIRQ (nIRQ);
}

void CSouthbridge::DisconnectIRQ (unsigned nIRQ)
{
	if (s_pThis != this)
	{
		s_pThis->DisconnectIRQ (nIRQ);

		return;
	}

	assert (s_bIsInitialized);

	assert (nIRQ & IRQ_FROM_RP1__MASK);
	unsigned nIRQLocal = nIRQ & IRQ_NUMBER__MASK;

	assert (nIRQLocal < RP1_IRQ_LINES);
	assert (m_apIRQHandler[nIRQLocal]);

	DisableIRQ (nIRQ);

	m_apIRQHandler[nIRQLocal] = nullptr;
	m_pParam[nIRQLocal] = nullptr;
}

void CSouthbridge::EnableIRQ (unsigned nIRQ)
{
	assert (nIRQ & IRQ_FROM_RP1__MASK);
	unsigned nIRQLocal = nIRQ & IRQ_NUMBER__MASK;
	assert (nIRQLocal < RP1_IRQ_LINES);

	assert (s_pThis);
	s_pThis->m_ulEnableMask |= BIT (nIRQLocal);

	if (nIRQ & IRQ_EDGE_TRIG__MASK)
	{
		s_pThis->m_bEdgeTriggered[nIRQLocal] = TRUE;

		write32 (INTC_REG_CLR + MSIX_CFG (nIRQLocal), MSIX_CFG_IACK_ENABLE);
	}
	else
	{
		s_pThis->m_bEdgeTriggered[nIRQLocal] = FALSE;

		write32 (INTC_REG_SET + MSIX_CFG (nIRQLocal), MSIX_CFG_IACK_ENABLE);
	}

	write32 (INTC_REG_SET + MSIX_CFG (nIRQLocal), MSIX_CFG_ENABLE);
}

void CSouthbridge::DisableIRQ (unsigned nIRQ)
{
	assert (nIRQ & IRQ_FROM_RP1__MASK);
	unsigned nIRQLocal = nIRQ & IRQ_NUMBER__MASK;
	assert (nIRQLocal < RP1_IRQ_LINES);

	write32 (INTC_REG_CLR + MSIX_CFG (nIRQLocal), MSIX_CFG_ENABLE);

	assert (s_pThis);
	s_pThis->m_ulEnableMask &= ~BIT (nIRQLocal);
}

CSouthbridge *CSouthbridge::Get (void)
{
	assert (s_pThis);
	return s_pThis;
}

#ifndef NDEBUG

void CSouthbridge::DumpStatus (void)
{
	if (s_pThis != this)
	{
		s_pThis->DumpStatus ();

		return;
	}

	if (!s_bIsInitialized)
	{
		LOGDBG ("RP1 device is not initialized");

		return;
	}

	assert (m_pPCIe);
	m_pPCIe->DumpStatus (RP1_PCIE_SLOT, RP1_PCIE_FUNC);

	LOGDBG ("IRQ HANDLER  PARAM    FL");
	for (unsigned nIRQ = 0; nIRQ < RP1_IRQ_LINES; nIRQ++)
	{
		if (m_apIRQHandler[nIRQ])
		{
			LOGDBG ("%02u: %08lX %08lX %c", nIRQ,
				(unsigned long) m_apIRQHandler[nIRQ],
				(unsigned long) m_pParam[nIRQ],
				m_bEdgeTriggered[nIRQ] ? 'e' : 'l');
		}
	}
}

#endif

void CSouthbridge::InterruptHandler (void *pParam)
{
	assert (s_pThis);
	assert (s_bIsInitialized);

	u64 ulStatus = (u64) read32 (INTC_INT_STAT_HIGH) << 32
			   | read32 (INTC_INT_STAT_LOW);
	ulStatus &= s_pThis->m_ulEnableMask;

	// LOGDBG ("IRQ (0x%lX)", ulStatus);

	for (unsigned nIRQ = 0; ulStatus; nIRQ++, ulStatus >>= 1)
	{
		if (ulStatus & 1)
		{
			if (s_pThis->m_apIRQHandler[nIRQ])
			{
				(*s_pThis->m_apIRQHandler[nIRQ]) (s_pThis->m_pParam[nIRQ]);
			}
			else
			{
				DisableIRQ (nIRQ | IRQ_FROM_RP1__MASK);

				LOGWARN ("Spurious IRQ (%u)", nIRQ);
			}

			if (!s_pThis->m_bEdgeTriggered[nIRQ])
			{
				write32 (INTC_REG_SET + MSIX_CFG (nIRQ), MSIX_CFG_IACK);
			}

			// TODO: break;
		}
	}
}
