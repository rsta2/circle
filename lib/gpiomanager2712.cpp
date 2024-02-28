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
#include <circle/rp1int.h>
#include <circle/bcm2712.h>
#include <circle/memio.h>
#include <assert.h>

#define GPIO0_BANKS	3

#define PCIE_INTS(bank)	(ARM_GPIO0_IO_BASE + (bank)*0x4000 + 0x124)

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
		assert (m_pInterrupt != 0);
		m_pInterrupt->DisconnectIRQ (RP1_IRQ_IO_BANK2);
		m_pInterrupt->DisconnectIRQ (RP1_IRQ_IO_BANK1);
		m_pInterrupt->DisconnectIRQ (RP1_IRQ_IO_BANK0);
	}

	m_pInterrupt = 0;
}

boolean CGPIOManager::Initialize (void)
{
	assert (!m_bIRQConnected);
	assert (m_pInterrupt != 0);
	m_pInterrupt->ConnectIRQ (RP1_IRQ_IO_BANK0, InterruptStub, this);
	m_pInterrupt->ConnectIRQ (RP1_IRQ_IO_BANK1, InterruptStub, this);
	m_pInterrupt->ConnectIRQ (RP1_IRQ_IO_BANK2, InterruptStub, this);
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

	unsigned nPin = 0;
	unsigned nBank = 0;
	unsigned nBankPin;
	do
	{
		u32 nEventStatus = read32 (PCIE_INTS (nBank));

		static unsigned BankPins[GPIO0_BANKS] = {28, 6, 20};

		nBankPin = 0;
		while (nBankPin < BankPins[nBank])
		{
			if (nEventStatus & 1)
			{
				goto PinFound;
			}
			nEventStatus >>= 1;

			++nPin;
			++nBankPin;
		}

		++nBank;
	}
	while (nBank < GPIO0_BANKS);

	if (nBank < GPIO0_BANKS)
	{
PinFound:
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
