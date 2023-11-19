//
// rp1.cpp
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
#include <circle/rp1.h>
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

CRP1 *CRP1::s_pThis = nullptr;

boolean CRP1::s_bIsInitialized = FALSE;

CRP1::CRP1 (CInterruptSystem *pInterrupt)
:	m_pInterrupt (pInterrupt),
	m_PCIe (pInterrupt),
	m_ulEnableMask (0)
{
	assert (!s_pThis);
	s_pThis = this;

	for (unsigned nIRQ = 0; nIRQ < RP1_IRQ_LINES; nIRQ++)
	{
		m_apIRQHandler[nIRQ] = nullptr;
		m_pParam[nIRQ] = nullptr;
		m_bEdgeTriggered[nIRQ] = FALSE;
	}
}

CRP1::~CRP1 (void)
{
	if (s_bIsInitialized)
	{
		assert (m_pInterrupt);
		m_pInterrupt->DisconnectIRQ (ARM_IRQ_PCIE_HOST_INTA);

		// TODO: Disable all enabled IRQs

		s_bIsInitialized = FALSE;
	}

	s_pThis = nullptr;
}

boolean CRP1::Initialize (void)
{
	assert (!s_bIsInitialized);

	if (!m_PCIe.Initialize ())
	{
		LOGERR ("Cannot initialize PCIe host bridge");

		return FALSE;
	}

	if (!m_PCIe.EnableDevice (RP1_PCI_CLASS_CODE, RP1_PCIE_SLOT, RP1_PCIE_FUNC))
	{
		LOGERR ("Cannot enable RP1 device");
	}

	assert (m_pInterrupt);
	m_pInterrupt->ConnectIRQ (ARM_IRQ_PCIE_HOST_INTA, InterruptHandler, this);

	s_bIsInitialized = TRUE;

	return TRUE;
}

void CRP1::ConnectIRQ (unsigned nIRQ, TIRQHandler *pHandler, void *pParam)
{
	assert (s_bIsInitialized);

	assert (nIRQ & IRQ_FROM_RP1__MASK);
	unsigned nIRQLocal = nIRQ & IRQ_NUMBER__MASK;

	assert (nIRQLocal < RP1_IRQ_LINES);
	assert (!m_apIRQHandler[nIRQLocal]);

	m_apIRQHandler[nIRQLocal] = pHandler;
	m_pParam[nIRQLocal] = pParam;

	EnableIRQ (nIRQ);
}

void CRP1::DisconnectIRQ (unsigned nIRQ)
{
	assert (s_bIsInitialized);

	assert (nIRQ & IRQ_FROM_RP1__MASK);
	unsigned nIRQLocal = nIRQ & IRQ_NUMBER__MASK;

	assert (nIRQLocal < RP1_IRQ_LINES);
	assert (m_apIRQHandler[nIRQLocal]);

	DisableIRQ (nIRQ);

	m_apIRQHandler[nIRQLocal] = nullptr;
	m_pParam[nIRQLocal] = nullptr;
}

void CRP1::EnableIRQ (unsigned nIRQ)
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

void CRP1::DisableIRQ (unsigned nIRQ)
{
	assert (nIRQ & IRQ_FROM_RP1__MASK);
	unsigned nIRQLocal = nIRQ & IRQ_NUMBER__MASK;
	assert (nIRQLocal < RP1_IRQ_LINES);

	write32 (INTC_REG_CLR + MSIX_CFG (nIRQLocal), MSIX_CFG_ENABLE);

	assert (s_pThis);
	s_pThis->m_ulEnableMask &= ~BIT (nIRQLocal);
}

CRP1 *CRP1::Get (void)
{
	assert (s_pThis);
	return s_pThis;
}

#ifndef NDEBUG

void CRP1::DumpStatus (void)
{
	if (!s_bIsInitialized)
	{
		LOGDBG ("RP1 device is not initialized");

		return;
	}

	m_PCIe.DumpStatus (RP1_PCIE_SLOT, RP1_PCIE_FUNC);

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

void CRP1::InterruptHandler (void *pParam)
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
