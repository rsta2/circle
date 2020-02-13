//
// gpiopin.cpp
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
#include <circle/gpiopin.h>
#include <circle/gpiomanager.h>
#include <circle/bcm2835.h>
#include <circle/memio.h>
#include <circle/machineinfo.h>
#include <circle/synchronize.h>
#include <circle/timer.h>
#include <assert.h>

CSpinLock CGPIOPin::s_SpinLock;

CGPIOPin::CGPIOPin (void)
:	m_nPin (GPIO_PINS),
	m_Mode (GPIOModeUnknown),
	m_pManager (0),
	m_pHandler (0),
	m_Interrupt (GPIOInterruptUnknown),
	m_Interrupt2 (GPIOInterruptUnknown)
{
}

CGPIOPin::CGPIOPin (unsigned nPin, TGPIOMode Mode, CGPIOManager *pManager)
:	m_nPin (GPIO_PINS),
	m_Mode (GPIOModeUnknown),
	m_pManager (pManager),
	m_pHandler (0),
	m_Interrupt (GPIOInterruptUnknown),
	m_Interrupt2 (GPIOInterruptUnknown)
{
	AssignPin (nPin);

	SetMode (Mode, TRUE);
}

CGPIOPin::~CGPIOPin (void)
{
	m_pHandler = 0;
	m_pManager = 0;
	
	m_nPin = GPIO_PINS;
}

void CGPIOPin::AssignPin (unsigned nPin)
{
	assert (m_nPin == GPIO_PINS);
	m_nPin = nPin;

	if (m_nPin >= GPIO_PINS)
	{
		m_nPin = CMachineInfo::Get ()->GetGPIOPin ((TGPIOVirtualPin) nPin);
	}
	assert (m_nPin < GPIO_PINS);

	m_nRegOffset = (m_nPin / 32) * 4;
	m_nRegMask = 1 << (m_nPin % 32);
}

void CGPIOPin::SetMode (TGPIOMode Mode, boolean bInitPin)
{
	assert (Mode < GPIOModeUnknown);
	m_Mode = Mode;

	PeripheralEntry ();

	if (GPIOModeAlternateFunction0 <= m_Mode && m_Mode <= GPIOModeAlternateFunction5)
	{
		if (bInitPin)
		{
			SetPullMode (GPIOPullModeOff);
		}

		SetAlternateFunction (m_Mode-GPIOModeAlternateFunction0);

		PeripheralExit ();

		return;
	}

	if (   bInitPin
	    && m_Mode == GPIOModeOutput)
	{
		SetPullMode (GPIOPullModeOff);
	}

	assert (m_nPin < GPIO_PINS);
	uintptr nSelReg = ARM_GPIO_GPFSEL0 + (m_nPin / 10) * 4;
	u32 nShift = (m_nPin % 10) * 3;

	s_SpinLock.Acquire ();
	u32 nValue = read32 (nSelReg);
	nValue &= ~(7 << nShift);
	nValue |= (m_Mode == GPIOModeOutput ? 1 : 0) << nShift;
	write32 (nSelReg, nValue);
	s_SpinLock.Release ();

	if (bInitPin)
	{
		switch (m_Mode)
		{
		case GPIOModeInput:
			SetPullMode (GPIOPullModeOff);
			break;

		case GPIOModeOutput:
			Write (LOW);
			break;

		case GPIOModeInputPullUp:
			SetPullMode (GPIOPullModeUp);
			break;

		case GPIOModeInputPullDown:
			SetPullMode (GPIOPullModeDown);
			break;

		default:
			break;
		}
	}

	PeripheralExit ();
}

void CGPIOPin::Write (unsigned nValue)
{
	assert (m_nPin < GPIO_PINS);

	// Output level can be set in input mode for subsequent switch to output
	assert (m_Mode < GPIOModeAlternateFunction0);

	PeripheralEntry ();

	assert (nValue == LOW || nValue == HIGH);
	m_nValue = nValue;

	uintptr nSetClrReg = (m_nValue ? ARM_GPIO_GPSET0 : ARM_GPIO_GPCLR0) + m_nRegOffset;

	write32 (nSetClrReg, m_nRegMask);

	PeripheralExit ();
}

unsigned CGPIOPin::Read (void) const
{
	assert (m_nPin < GPIO_PINS);

	assert (   m_Mode == GPIOModeInput
		|| m_Mode == GPIOModeInputPullUp
		|| m_Mode == GPIOModeInputPullDown);
	
	PeripheralEntry ();

	unsigned nResult = read32 (ARM_GPIO_GPLEV0 + m_nRegOffset) & m_nRegMask ? HIGH : LOW;
	
	PeripheralExit ();

	return nResult;
}

void CGPIOPin::Invert (void)
{
	assert (m_Mode == GPIOModeOutput);

	Write (m_nValue ^ 1);
}

void CGPIOPin::ConnectInterrupt (TGPIOInterruptHandler *pHandler, void *pParam, boolean bAutoAck)
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

	assert (m_pManager != 0);
	m_pManager->ConnectInterrupt (this);
}

void CGPIOPin::DisconnectInterrupt (void)
{
	assert (   m_Mode == GPIOModeInput
		|| m_Mode == GPIOModeInputPullUp
		|| m_Mode == GPIOModeInputPullDown);

	assert (m_Interrupt == GPIOInterruptUnknown);
	assert (m_Interrupt2 == GPIOInterruptUnknown);

	assert (m_pHandler != 0);
	m_pHandler = 0;

	assert (m_pManager != 0);
	m_pManager->DisconnectInterrupt (this);
}

void CGPIOPin::EnableInterrupt (TGPIOInterrupt Interrupt)
{
	assert (   m_Mode == GPIOModeInput
		|| m_Mode == GPIOModeInputPullUp
		|| m_Mode == GPIOModeInputPullDown);
	assert (m_pHandler != 0);

	assert (m_Interrupt == GPIOInterruptUnknown);
	assert (Interrupt < GPIOInterruptUnknown);
	assert (Interrupt != m_Interrupt2);
	m_Interrupt = Interrupt;

	uintptr nReg =   ARM_GPIO_GPREN0
			+ m_nRegOffset
			+ (Interrupt - GPIOInterruptOnRisingEdge) * 12;

	s_SpinLock.Acquire ();
	write32 (nReg, read32 (nReg) | m_nRegMask);
	s_SpinLock.Release ();
}
	

void CGPIOPin::DisableInterrupt (void)
{
	assert (   m_Mode == GPIOModeInput
		|| m_Mode == GPIOModeInputPullUp
		|| m_Mode == GPIOModeInputPullDown);

	assert (m_Interrupt < GPIOInterruptUnknown);

	uintptr nReg =   ARM_GPIO_GPREN0
			+ m_nRegOffset
			+ (m_Interrupt - GPIOInterruptOnRisingEdge) * 12;

	s_SpinLock.Acquire ();
	write32 (nReg, read32 (nReg) & ~m_nRegMask);
	s_SpinLock.Release ();

	m_Interrupt = GPIOInterruptUnknown;
}

void CGPIOPin::EnableInterrupt2 (TGPIOInterrupt Interrupt)
{
	assert (   m_Mode == GPIOModeInput
		|| m_Mode == GPIOModeInputPullUp
		|| m_Mode == GPIOModeInputPullDown);
	assert (m_pHandler != 0);

	assert (m_Interrupt2 == GPIOInterruptUnknown);
	assert (Interrupt < GPIOInterruptUnknown);
	assert (Interrupt != m_Interrupt);
	m_Interrupt2 = Interrupt;

	uintptr nReg =   ARM_GPIO_GPREN0
			+ m_nRegOffset
			+ (Interrupt - GPIOInterruptOnRisingEdge) * 12;

	s_SpinLock.Acquire ();
	write32 (nReg, read32 (nReg) | m_nRegMask);
	s_SpinLock.Release ();
}


void CGPIOPin::DisableInterrupt2 (void)
{
	assert (   m_Mode == GPIOModeInput
		|| m_Mode == GPIOModeInputPullUp
		|| m_Mode == GPIOModeInputPullDown);

	assert (m_Interrupt2 < GPIOInterruptUnknown);

	uintptr nReg =   ARM_GPIO_GPREN0
			+ m_nRegOffset
			+ (m_Interrupt2 - GPIOInterruptOnRisingEdge) * 12;

	s_SpinLock.Acquire ();
	write32 (nReg, read32 (nReg) & ~m_nRegMask);
	s_SpinLock.Release ();

	m_Interrupt2 = GPIOInterruptUnknown;
}

void CGPIOPin::AcknowledgeInterrupt (void)
{
	assert (m_pHandler != 0);
	assert (!m_bAutoAck);

	PeripheralEntry ();

	write32 (ARM_GPIO_GPEDS0 + m_nRegOffset, m_nRegMask);

	PeripheralExit ();
}

void CGPIOPin::WriteAll (u32 nValue, u32 nMask)
{
	PeripheralEntry ();

	u32 nClear = ~nValue & nMask;
	if (nClear != 0)
	{
		write32 (ARM_GPIO_GPCLR0, nClear);
	}

	u32 nSet = nValue & nMask;
	if (nSet != 0)
	{
		write32 (ARM_GPIO_GPSET0, nSet);
	}

	PeripheralExit ();
}

u32 CGPIOPin::ReadAll (void)
{
	PeripheralEntry ();

	u32 nResult = read32 (ARM_GPIO_GPLEV0);

	PeripheralExit ();

	return nResult;
}

void CGPIOPin::SetPullMode (TGPIOPullMode Mode)
{
	s_SpinLock.Acquire ();

	PeripheralEntry ();

#if RASPPI <= 3
	// See: http://www.raspberrypi.org/forums/viewtopic.php?t=163352&p=1059178#p1059178
	uintptr nClkReg = ARM_GPIO_GPPUDCLK0 + m_nRegOffset;

	assert (Mode <= 2);
	write32 (ARM_GPIO_GPPUD, Mode);
	CTimer::SimpleusDelay (5);		// 1us should be enough, but to be sure
	write32 (nClkReg, m_nRegMask);
	CTimer::SimpleusDelay (5);		// 1us should be enough, but to be sure
	write32 (ARM_GPIO_GPPUD, 0);
	write32 (nClkReg, 0);
#else
	assert (m_nPin < GPIO_PINS);
	uintptr nModeReg = ARM_GPIO_GPPUPPDN0 + (m_nPin / 16) * 4;
	unsigned nShift = (m_nPin % 16) * 2;

	assert (Mode <= 2);
	static const unsigned ModeMap[3] = {0, 2, 1};

	u32 nValue = read32 (nModeReg);
	nValue &= ~(3 << nShift);
	nValue |= ModeMap[Mode] << nShift;
	write32 (nModeReg, nValue);
#endif

	PeripheralExit ();

	s_SpinLock.Release ();
}

void CGPIOPin::SetAlternateFunction (unsigned nFunction)
{
	assert (m_nPin < GPIO_PINS);
	uintptr nSelReg = ARM_GPIO_GPFSEL0 + (m_nPin / 10) * 4;
	unsigned nShift = (m_nPin % 10) * 3;

	assert (nFunction <= 5);
	static const unsigned FunctionMap[6] = {4, 5, 6, 7, 3, 2};
	
	s_SpinLock.Acquire ();
	u32 nValue = read32 (nSelReg);
	nValue &= ~(7 << nShift);
	nValue |= FunctionMap[nFunction] << nShift;
	write32 (nSelReg, nValue);
	s_SpinLock.Release ();
}

void CGPIOPin::InterruptHandler (void)
{
	assert (   m_Mode == GPIOModeInput
		|| m_Mode == GPIOModeInputPullUp
		|| m_Mode == GPIOModeInputPullDown);
	assert (   m_Interrupt  < GPIOInterruptUnknown
		|| m_Interrupt2 < GPIOInterruptUnknown);

	assert (m_pHandler != 0);
	(*m_pHandler) (m_pParam);
}

void CGPIOPin::DisableAllInterrupts (unsigned nPin)
{
	assert (nPin < GPIO_PINS);

	uintptr nReg = ARM_GPIO_GPREN0 + (nPin / 32) * 4;
	u32 nMask = 1 << (nPin % 32);

	s_SpinLock.Acquire ();

	for (; nReg < ARM_GPIO_GPAFEN0 + 4; nReg += 12)
	{
		write32 (nReg, read32 (nReg) & ~nMask);
	}

	s_SpinLock.Release ();
}
