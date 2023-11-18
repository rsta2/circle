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
#include <circle/gpiomanager.h>
#include <circle/bcm2712.h>
#include <circle/memio.h>
#include <circle/logger.h>
#include <circle/macros.h>
#include <circle/rp1.h>
#include <assert.h>

// TODO: Support alternate functions
// TODO: Support IO_BANK1 and IO_BANK2
// TODO: Support non-RP1 GPIO pins
// TODO: GPIOInterruptOn[Rising|Falling]Edge does not work
// TODO: Use SET/CLR registers (if available?)

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
	#define CTRL_IRQMASK_EDGE_LOW__MASK	BIT (20)
	#define CTRL_IRQMASK_EDGE_HIGH__MASK	BIT (21)
	#define CTRL_IRQMASK_LEVEL_LOW__MASK	BIT (22)
	#define CTRL_IRQMASK_LEVEL_HIGH__MASK	BIT (23)
	#define CTRL_IRQMASK_F_EDGE_LOW__MASK	BIT (24)
	#define CTRL_IRQMASK_F_EDGE_HIGH__MASK	BIT (25)
	#define CTRL_IRQMASK_DB_LEVEL_LOW__MASK	BIT (26)
	#define CTRL_IRQMASK_DB_LEVEL_HIGH__MASK BIT (27)
	#define CTRL_IRQRESET__MASK		BIT (28)
	#define CTRL_IRQOVER__SHIFT		30
	#define CTRL_IRQOVER__MASK		(3 << 30)
		#define IRQOVER_DONT_INVERT	0
		#define IRQOVER_INVERT		1
		#define IRQOVER_LOW		2
		#define IRQOVER_HIGH		3
#define PCIE_INTE				(ARM_GPIO0_IO_BASE + 0x11C)
#define PCIE_INTF				(ARM_GPIO0_IO_BASE + 0x120)
#define PCIE_INTS				(ARM_GPIO0_IO_BASE + 0x124)
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

LOGMODULE ("gpio2712");

CSpinLock CGPIOPin::s_SpinLock;

u32 CGPIOPin::s_nInterruptMap[GPIOInterruptUnknown] =
{
	CTRL_IRQMASK_F_EDGE_HIGH__MASK,		// RisingEdge
	CTRL_IRQMASK_F_EDGE_LOW__MASK,		// FallingEdge
	CTRL_IRQMASK_LEVEL_HIGH__MASK,		// HighLevel
	CTRL_IRQMASK_LEVEL_LOW__MASK,		// LowLevel
	CTRL_IRQMASK_EDGE_HIGH__MASK,		// AsyncRisingEdge
	CTRL_IRQMASK_EDGE_LOW__MASK,		// AsyncFallingEdge
	CTRL_IRQMASK_DB_LEVEL_HIGH__MASK,	// DebouncedHighLevel
	CTRL_IRQMASK_DB_LEVEL_LOW__MASK		// DebouncedLowLevel
};

CGPIOPin::CGPIOPin (void)
:	m_nPin (GPIO_PINS),
	m_Mode (GPIOModeUnknown),
	m_nValue (LOW),
	m_pManager (nullptr),
	m_pHandler (nullptr),
	m_Interrupt (GPIOInterruptUnknown),
	m_Interrupt2 (GPIOInterruptUnknown)
{
}

CGPIOPin::CGPIOPin (unsigned nPin, TGPIOMode Mode, CGPIOManager *pManager)
:	m_nPin (GPIO_PINS),
	m_Mode (GPIOModeUnknown),
	m_nValue (LOW),
	m_pManager (pManager),
	m_pHandler (nullptr),
	m_Interrupt (GPIOInterruptUnknown),
	m_Interrupt2 (GPIOInterruptUnknown)
{
	AssignPin (nPin);

	SetMode (Mode, TRUE);
}

CGPIOPin::~CGPIOPin (void)
{
	m_pHandler = nullptr;
	m_pManager = nullptr;

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

	// TODO: Acquire m_SpinLock?

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

void CGPIOPin::ConnectInterrupt (TGPIOInterruptHandler *pHandler, void *pParam, boolean bAutoAck)
{
	assert (   m_Mode == GPIOModeInput
		|| m_Mode == GPIOModeInputPullUp
		|| m_Mode == GPIOModeInputPullDown);

	assert (m_Interrupt == GPIOInterruptUnknown);
	assert (m_Interrupt2 == GPIOInterruptUnknown);

	assert (pHandler);
	assert (!m_pHandler);
	m_pHandler = pHandler;

	m_pParam = pParam;

	m_bAutoAck = bAutoAck;

	assert (m_pManager);
	m_pManager->ConnectInterrupt (this);

	assert (m_nPin < GPIO_PINS);
	s_SpinLock.Acquire ();
	write32 (PCIE_INTE, read32 (PCIE_INTE) | BIT (m_nPin));
	s_SpinLock.Release ();
}

void CGPIOPin::DisconnectInterrupt (void)
{
	assert (   m_Mode == GPIOModeInput
		|| m_Mode == GPIOModeInputPullUp
		|| m_Mode == GPIOModeInputPullDown);

	assert (m_Interrupt == GPIOInterruptUnknown);
	assert (m_Interrupt2 == GPIOInterruptUnknown);

	assert (m_pHandler);
	m_pHandler = nullptr;

	assert (m_nPin < GPIO_PINS);
	s_SpinLock.Acquire ();
	write32 (PCIE_INTE, read32 (PCIE_INTE) & ~BIT (m_nPin));
	s_SpinLock.Release ();

	assert (m_pManager);
	m_pManager->DisconnectInterrupt (this);
}

void CGPIOPin::EnableInterrupt (TGPIOInterrupt Interrupt)
{
	assert (   m_Mode == GPIOModeInput
		|| m_Mode == GPIOModeInputPullUp
		|| m_Mode == GPIOModeInputPullDown);
	assert (m_pHandler);

	assert (m_Interrupt == GPIOInterruptUnknown);
	assert (Interrupt < GPIOInterruptUnknown);
	assert (Interrupt != m_Interrupt2);
	m_Interrupt = Interrupt;

	uintptr nReg = GPIO0_CTRL (m_nPin);

	m_SpinLock.Acquire ();
	write32 (nReg, read32 (nReg) | s_nInterruptMap[Interrupt]);
	m_SpinLock.Release ();
}


void CGPIOPin::DisableInterrupt (void)
{
	assert (   m_Mode == GPIOModeInput
		|| m_Mode == GPIOModeInputPullUp
		|| m_Mode == GPIOModeInputPullDown);

	assert (m_Interrupt < GPIOInterruptUnknown);

	uintptr nReg = GPIO0_CTRL (m_nPin);

	m_SpinLock.Acquire ();
	write32 (nReg, read32 (nReg) & ~s_nInterruptMap[m_Interrupt]);
	m_SpinLock.Release ();

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

	uintptr nReg = GPIO0_CTRL (m_nPin);

	m_SpinLock.Acquire ();
	write32 (nReg, read32 (nReg) | s_nInterruptMap[Interrupt]);
	m_SpinLock.Release ();
}


void CGPIOPin::DisableInterrupt2 (void)
{
	assert (   m_Mode == GPIOModeInput
		|| m_Mode == GPIOModeInputPullUp
		|| m_Mode == GPIOModeInputPullDown);

	assert (m_Interrupt2 < GPIOInterruptUnknown);

	uintptr nReg = GPIO0_CTRL (m_nPin);

	m_SpinLock.Acquire ();
	write32 (nReg, read32 (nReg) & ~s_nInterruptMap[m_Interrupt2]);
	m_SpinLock.Release ();

	m_Interrupt2 = GPIOInterruptUnknown;
}

void CGPIOPin::AcknowledgeInterrupt (void)
{
	assert (m_pHandler);

	// only when edge triggered interrupt is active
	if (   (   m_Interrupt == GPIOInterruptOnHighLevel
		|| m_Interrupt == GPIOInterruptOnLowLevel
		|| m_Interrupt == GPIOInterruptUnknown)
	    && (   m_Interrupt2 == GPIOInterruptOnHighLevel
		|| m_Interrupt2 == GPIOInterruptOnLowLevel
		|| m_Interrupt2 == GPIOInterruptUnknown))
	{
		return;
	}

	uintptr nReg = GPIO0_CTRL (m_nPin);

	m_SpinLock.Acquire ();
	write32 (nReg, read32 (nReg) | CTRL_IRQRESET__MASK);
	// assert (!(read32 (nReg) & CTRL_IRQRESET__MASK));
	m_SpinLock.Release ();
}

#ifndef NDEBUG

void CGPIOPin::DumpStatus (void)
{
	if (CRP1::IsInitialized ())
	{
		LOGDBG ("GIO STATUS   CTRL     PADS");

		for (unsigned nPin = 0; nPin < GPIO_PINS; nPin++)
		{
			LOGDBG ("%02u: %08X %08X %02X",
				nPin,
				read32 (GPIO0_STATUS (nPin)),
				read32 (GPIO0_CTRL (nPin)),
				read32 (PADS0_CTRL (nPin)));
		}
	}
	else
	{
		LOGDBG ("RP1 is not initialized");
	}
}

#endif

void CGPIOPin::InterruptHandler (void)
{
	assert (   m_Mode == GPIOModeInput
		|| m_Mode == GPIOModeInputPullUp
		|| m_Mode == GPIOModeInputPullDown);
	assert (   m_Interrupt  < GPIOInterruptUnknown
		|| m_Interrupt2 < GPIOInterruptUnknown);

	assert (m_pHandler);
	(*m_pHandler) (m_pParam);
}

void CGPIOPin::DisableAllInterrupts (unsigned nPin)
{
	assert (nPin < GPIO_PINS);

	s_SpinLock.Acquire ();
	write32 (PCIE_INTE, read32 (PCIE_INTE) & ~BIT (nPin));
	s_SpinLock.Release ();
}
