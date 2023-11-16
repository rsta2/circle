//
// gpiopin2712.cpp
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
#include <circle/gpiopin.h>
#include <circle/bcm2712.h>
#include <circle/memio.h>
#include <circle/macros.h>
#include <circle/rp1.h>
#include <assert.h>

// TODO: Support non-RP1 GPIO pins etc.

// See: Raspberry Pi RP1 Peripherals
//      https://datasheets.raspberrypi.com/rp1/rp1-peripherals.pdf

#define GPIO0_STATUS(pin)			(ARM_GPIO0_IO_BASE + (pin) * 8)
	#define STATUS_INISDIRECT__MASK		BIT (16)
	#define STATUS_INFROMPAD__MASK		BIT (17)
	#define STATUS_INFILTERED__MASK		BIT (18)
	#define STATUS_INTOPERI__MASK		BIT (19)
#define GPIO0_CTRL(pin)				(ARM_GPIO0_IO_BASE + (pin) * 8 + 4)
	#define CTRL_FUNCSEL__SHIFT		0
	#define CTRL_FUNCSEL__MASK		(0x1F << 0)
		#define FUNCSEL_NULL		31
		#define FUNCSEL_ALT_MAX		8
	#define CTRL_FILTER_CONST_M__SHIFT	5
	#define CTRL_FILTER_CONST_M__MASK	(0x7F << 5)
		#define FILTER_CONST_M_DEFAULT	4
	#define CTRL_OUTOVER__SHIFT		12
	#define CTRL_OUTOVER__MASK		(3 << 12)
		#define OUTOVER_FUNCSEL		0
		#define OUTOVER_FUNCSEL_INVERT	1
		#define OUTOVER_LOW		2
		#define OUTOVER_HIGH		3
	#define CTRL_OEOVER__SHIFT		14
	#define CTRL_OEOVER__MASK		(3 << 14)
		#define OEOVER_FUNCSEL		0
		#define OEOVER_FUNCSEL_INVERT	1
		#define OEOVER_DISABLE		2
		#define OEOVER_ENABLE		3
	#define CTRL_INOVER__SHIFT		16
	#define CTRL_INOVER__MASK		(3 << 16)
		#define INOVER_DONT_INVERT	0
		#define INOVER_INVERT		1
		#define INOVER_LOW		2
		#define INOVER_HIGH		3
#define PADS0_CTRL(pin)				(ARM_GPIO0_PADS_BASE + 4 + (pin) * 4)
	#define PADS_SLEWFAST__MASK		BIT (0)
	#define PADS_SCHMITT__MASK		BIT (1)
	#define PADS_PDE__MASK			BIT (2)
	#define PADS_PUE__MASK			BIT (3)
	#define PADS_DRIVE__SHIFT		4
	#define PADS_DRIVE__MASK		(3 << 4)
		#define DRIVE_2MA		0
		#define DRIVE_4MA		1
		#define DRIVE_8MA		2
		#define DRIVE_12MA		3
	#define PADS_IE__MASK			BIT (6)
	#define PADS_OD__MASK			BIT (7)

CGPIOPin::CGPIOPin (void)
:	m_nPin (GPIO_PINS),
	m_Mode (GPIOModeUnknown),
	m_nValue (LOW)
{
}

CGPIOPin::CGPIOPin (unsigned nPin, TGPIOMode Mode)
:	m_nPin (GPIO_PINS),
	m_Mode (GPIOModeUnknown),
	m_nValue (LOW)
{
	AssignPin (nPin);

	SetMode (Mode);
}

CGPIOPin::~CGPIOPin (void)
{
	m_nPin = GPIO_PINS;
}

void CGPIOPin::AssignPin (unsigned nPin)
{
	assert (m_nPin == GPIO_PINS);
	m_nPin = nPin;
	assert (m_nPin < GPIO_PINS);
}

void CGPIOPin::SetMode (TGPIOMode Mode, boolean bInitPin)
{
	assert (m_nPin < GPIO_PINS);
	assert (bInitPin);			// TODO
	assert (CRP1::IsInitialized ());

	m_Mode = Mode;

	switch (m_Mode)
	{
	case GPIOModeInput:
		write32 (GPIO0_CTRL (m_nPin),
			   (FUNCSEL_NULL << CTRL_FUNCSEL__SHIFT)
			 | (FILTER_CONST_M_DEFAULT << CTRL_FILTER_CONST_M__SHIFT)
			 | (OEOVER_DISABLE << CTRL_OEOVER__SHIFT));
		write32 (PADS0_CTRL (m_nPin), PADS_SCHMITT__MASK | PADS_IE__MASK);
		break;

	case GPIOModeOutput:
		write32 (GPIO0_CTRL (m_nPin),
			   (FUNCSEL_NULL << CTRL_FUNCSEL__SHIFT)
			 | (OUTOVER_LOW << CTRL_OUTOVER__SHIFT)
			 | (OEOVER_ENABLE << CTRL_OEOVER__SHIFT));
		write32 (PADS0_CTRL (m_nPin), (DRIVE_12MA << PADS_DRIVE__SHIFT));
		break;

	case GPIOModeInputPullUp:
		write32 (GPIO0_CTRL (m_nPin),
			   (FUNCSEL_NULL << CTRL_FUNCSEL__SHIFT)
			 | (FILTER_CONST_M_DEFAULT << CTRL_FILTER_CONST_M__SHIFT)
			 | (OEOVER_DISABLE << CTRL_OEOVER__SHIFT));
		write32 (PADS0_CTRL (m_nPin), PADS_SCHMITT__MASK | PADS_PUE__MASK | PADS_IE__MASK);
		break;

	case GPIOModeInputPullDown:
		write32 (GPIO0_CTRL (m_nPin),
			   (FUNCSEL_NULL << CTRL_FUNCSEL__SHIFT)
			 | (FILTER_CONST_M_DEFAULT << CTRL_FILTER_CONST_M__SHIFT)
			 | (OEOVER_DISABLE << CTRL_OEOVER__SHIFT));
		write32 (PADS0_CTRL (m_nPin), PADS_SCHMITT__MASK | PADS_PDE__MASK | PADS_IE__MASK);
		break;

	default:
		assert (0);
		break;
	}
}

void CGPIOPin::SetPullMode (TGPIOPullMode Mode)
{
	assert (m_nPin < GPIO_PINS);
	assert (CRP1::IsInitialized ());

	u32 nPads = read32 (PADS0_CTRL (m_nPin));
	nPads &= ~(PADS_PDE__MASK | PADS_PUE__MASK);

	switch (Mode)
	{
	case GPIOPullModeOff:
		break;

	case GPIOPullModeDown:
		nPads |= PADS_PDE__MASK;
		break;

	case GPIOPullModeUp:
		nPads |= PADS_PUE__MASK;
		break;

	default:
		assert (0);
		break;
	}

	write32 (PADS0_CTRL (m_nPin), nPads);
}

void CGPIOPin::Write (unsigned nValue)
{
	assert (m_nPin < GPIO_PINS);
	assert (m_Mode == GPIOModeOutput);
	assert (nValue <= HIGH);

	m_nValue = nValue;

	write32 (GPIO0_CTRL (m_nPin),
		   (FUNCSEL_NULL << CTRL_FUNCSEL__SHIFT)
		 | ((nValue ? OUTOVER_HIGH : OUTOVER_LOW) << CTRL_OUTOVER__SHIFT)
		 | (OEOVER_ENABLE << CTRL_OEOVER__SHIFT));
}

unsigned CGPIOPin::Read (void) const
{
	assert (m_nPin < GPIO_PINS);
	assert (   m_Mode == GPIOModeInput
		|| m_Mode == GPIOModeInputPullUp
		|| m_Mode == GPIOModeInputPullDown);

	return !!(read32 (GPIO0_STATUS (m_nPin)) & STATUS_INFILTERED__MASK);
}

void CGPIOPin::Invert (void)
{
	Write (m_nValue ^ 1);
}
