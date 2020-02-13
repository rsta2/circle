//
// gpiopinfiq.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
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
#include <circle/gpiopinfiq.h>
#include <circle/bcm2835.h>
#include <circle/memio.h>
#include <circle/synchronize.h>
#include <assert.h>

CGPIOPinFIQ::CGPIOPinFIQ (unsigned nPin, TGPIOMode Mode, CInterruptSystem *pInterrupt)
:	CGPIOPin (nPin, Mode),
	m_pInterrupt (pInterrupt)
{
}

CGPIOPinFIQ::~CGPIOPinFIQ (void)
{
	m_pInterrupt = 0;
}

void CGPIOPinFIQ::ConnectInterrupt (TGPIOInterruptHandler *pHandler, void *pParam, boolean bAutoAck)
{
	assert (   m_Mode == GPIOModeInput
		|| m_Mode == GPIOModeInputPullUp
		|| m_Mode == GPIOModeInputPullDown);

	assert (m_Interrupt == GPIOInterruptUnknown);
	assert (m_Interrupt2 == GPIOInterruptUnknown);

	assert (pHandler != 0);
	assert (m_pHandler == 0);
	m_pHandler = pHandler;

	m_pParam = pParam;

	m_bAutoAck = bAutoAck;

	assert (m_pInterrupt != 0);
	m_pInterrupt->ConnectFIQ (ARM_FIQ_GPIO3, FIQHandler, this);
}

void CGPIOPinFIQ::DisconnectInterrupt (void)
{
	assert (   m_Mode == GPIOModeInput
		|| m_Mode == GPIOModeInputPullUp
		|| m_Mode == GPIOModeInputPullDown);

	assert (m_Interrupt == GPIOInterruptUnknown);
	assert (m_Interrupt2 == GPIOInterruptUnknown);

	assert (m_pHandler != 0);
	m_pHandler = 0;

	assert (m_pInterrupt != 0);
	m_pInterrupt->DisconnectFIQ ();
}

void CGPIOPinFIQ::FIQHandler (void *pParam)
{
	CGPIOPinFIQ *pThis = (CGPIOPinFIQ *) pParam;

	if (pThis->m_bAutoAck)
	{
		PeripheralEntry ();
		write32 (ARM_GPIO_GPEDS0 + pThis->m_nRegOffset, pThis->m_nRegMask);
		PeripheralExit ();
	}

	(*pThis->m_pHandler) (pThis->m_pParam);
}
