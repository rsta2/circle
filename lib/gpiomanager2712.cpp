//
// gpiomanager2712.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2023  R. Stange <rsta2@o2online.de>
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
#include <circle/gpiomanager.h>
#include <circle/rp1.h>
#include <circle/rp1int.h>
#include <circle/bcm2712.h>
#include <circle/memio.h>
#include <assert.h>

#define GPIO_IRQ	RP1_IRQ_IO_BANK0

#define PCIE_INTS	(ARM_GPIO0_IO_BASE + 0x124)

CGPIOManager::CGPIOManager (CInterruptSystem *pInterrupt)
:	m_pInterrupt (pInterrupt),
	m_bIRQConnected (FALSE)
{
	for (unsigned nPin = 0; nPin < GPIO_PINS; nPin++)
	{
		m_apPin[nPin] = 0;
	}
}

CGPIOManager::~CGPIOManager (void)
{
#ifndef NDEBUG
	for (unsigned nPin = 0; nPin < GPIO_PINS; nPin++)
	{
		assert (m_apPin[nPin] == 0);
	}
#endif

	if (m_bIRQConnected)
	{
		// TODO: assert (m_pInterrupt != 0);
		CRP1::Get ()->DisconnectIRQ (GPIO_IRQ);
	}

	m_pInterrupt = 0;
}

boolean CGPIOManager::Initialize (void)
{
	assert (!m_bIRQConnected);
	// TODO: assert (m_pInterrupt != 0);
	CRP1::Get ()->ConnectIRQ (GPIO_IRQ, InterruptStub, this);
	m_bIRQConnected = TRUE;

	return TRUE;
}

void CGPIOManager::ConnectInterrupt (CGPIOPin *pPin)
{
	assert (m_bIRQConnected);

	assert (pPin != 0);
	unsigned nPin = pPin->m_nPin;
	assert (nPin < GPIO_PINS);

	assert (m_apPin[nPin] == 0);
	m_apPin[nPin] = pPin;
}


void CGPIOManager::DisconnectInterrupt (CGPIOPin *pPin)
{
	assert (m_bIRQConnected);

	assert (pPin != 0);
	unsigned nPin = pPin->m_nPin;
	assert (nPin < GPIO_PINS);

	assert (m_apPin[nPin] != 0);
	m_apPin[nPin] = 0;
}

void CGPIOManager::InterruptHandler (void)
{
	assert (m_bIRQConnected);

	u32 nEventStatus = read32 (PCIE_INTS);

	unsigned nPin = 0;
	while (nPin < GPIO_PINS)
	{
		if (nEventStatus & 1)
		{
			break;
		}
		nEventStatus >>= 1;

		++nPin;
	}

	if (nPin < GPIO_PINS)
	{
		CGPIOPin *pPin = m_apPin[nPin];
		if (pPin)
		{
			pPin->InterruptHandler ();

			if (pPin->m_bAutoAck)
			{
				pPin->AcknowledgeInterrupt ();
			}
		}
		else
		{
			// disable all interrupt sources
			CGPIOPin::DisableAllInterrupts (nPin);

			// TODO: pPin->AcknowledgeInterrupt ();
		}
	}
}

void CGPIOManager::InterruptStub (void *pParam)
{
	CGPIOManager *pThis = (CGPIOManager *) pParam;
	assert (pParam != 0);

	pThis->InterruptHandler ();
}
