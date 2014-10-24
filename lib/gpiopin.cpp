//
// gpiopin.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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
#include <circle/bcm2835.h>
#include <circle/memio.h>
#include <circle/synchronize.h>
#include <circle/timer.h>
#include <assert.h>

#define GPIO_PINS	54

CGPIOPin::CGPIOPin (unsigned nPin, TGPIOMode Mode)
:	m_nPin (nPin),
	m_Mode (GPIOModeUnknown)
{
	assert (m_nPin < GPIO_PINS);
	
	SetMode (Mode);
}

CGPIOPin::~CGPIOPin (void)
{
	m_nPin = GPIO_PINS;
}

void CGPIOPin::SetMode (TGPIOMode Mode)
{
	assert (Mode < GPIOModeUnknown);
	m_Mode = Mode;

	DataMemBarrier ();

	if (GPIOModeAlternateFunction0 <= m_Mode && m_Mode <= GPIOModeAlternateFunction5)
	{
		SetAlternateFunction (m_Mode-GPIOModeAlternateFunction0);

		DataMemBarrier ();

		return;
	}

	if (m_Mode == GPIOModeOutput)		// TODO: Is this required?
	{
		SetPullUpMode (0);
	}

	assert (m_nPin < GPIO_PINS);
	unsigned nSelReg = ARM_GPIO_GPFSEL0 + (m_nPin / 10) * 4;
	unsigned nShift = (m_nPin % 10) * 3;

	unsigned nValue = read32 (nSelReg);
	nValue &= ~(7 << nShift);
	nValue |= (m_Mode == GPIOModeOutput ? 1 : 0) << nShift;
	write32 (nSelReg, nValue);

	switch (m_Mode)
	{
	case GPIOModeOutput:
		Write (LOW);
		break;

	case GPIOModeInputPullUp:
		SetPullUpMode (2);
		break;

	case GPIOModeInputPullDown:
		SetPullUpMode (1);
		break;

	default:
		break;
	}

	DataMemBarrier ();
}

void CGPIOPin::Write (unsigned nValue)
{
	assert (m_Mode == GPIOModeOutput);

	DataMemBarrier ();

	assert (nValue == LOW || nValue == HIGH);
	m_nValue = nValue;

	assert (m_nPin < GPIO_PINS);
	unsigned nSetClrReg = (m_nValue ? ARM_GPIO_GPSET0 : ARM_GPIO_GPCLR0) + (m_nPin / 32) * 4;
	unsigned nShift = m_nPin % 32;

	write32 (nSetClrReg, 1 << nShift);

	DataMemBarrier ();
}

unsigned CGPIOPin::Read (void) const
{
	assert (   m_Mode == GPIOModeInput
		|| m_Mode == GPIOModeInputPullUp
		|| m_Mode == GPIOModeInputPullDown);
	
	DataMemBarrier ();

	assert (m_nPin < GPIO_PINS);
	unsigned nLevReg = ARM_GPIO_GPLEV0 + (m_nPin / 32) * 4;
	unsigned nShift = m_nPin % 32;

	unsigned nResult = read32 (nLevReg) & (1 << nShift) ? HIGH : LOW;
	
	DataMemBarrier ();

	return nResult;
}

void CGPIOPin::Invert (void)
{
	assert (m_Mode == GPIOModeOutput);

	Write (m_nValue ^ 1);
}

void CGPIOPin::SetPullUpMode (unsigned nMode)
{
	assert (m_nPin < GPIO_PINS);
	unsigned nClkReg = ARM_GPIO_GPPUDCLK0 + (m_nPin / 32) * 4;
	unsigned nShift = m_nPin % 32;
	
	assert (nMode <= 2);
	write32 (ARM_GPIO_GPPUD, nMode);
	CTimer::SimpleusDelay (150);		// TODO: 150 cycles (?)
	write32 (nClkReg, 1 << nShift);
	CTimer::SimpleusDelay (150);		// TODO: 150 cycles (?)
	write32 (ARM_GPIO_GPPUD, 0);
	write32 (nClkReg, 0);
}

void CGPIOPin::SetAlternateFunction (unsigned nFunction)
{
	assert (m_nPin < GPIO_PINS);
	unsigned nSelReg = ARM_GPIO_GPFSEL0 + (m_nPin / 10) * 4;
	unsigned nShift = (m_nPin % 10) * 3;

	assert (nFunction <= 5);
	static const unsigned FunctionMap[6] = {4, 5, 6, 7, 3, 2};
	
	unsigned nValue = read32 (nSelReg);
	nValue &= ~(7 << nShift);
	nValue |= FunctionMap[nFunction] << nShift;
	write32 (nSelReg, nValue);
}
