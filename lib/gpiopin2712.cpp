//
// gpiopin2712.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2023-2024  R. Stange <rsta2@o2online.de>
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
#include <circle/machineinfo.h>
#include <circle/logger.h>
#include <circle/macros.h>
#include <circle/southbridge.h>
#include <circle/synchronize.h>
#include <assert.h>

// TODO: Support non-RP1 GPIO pins

// See: Raspberry Pi RP1 Peripherals
//      https://datasheets.raspberrypi.com/rp1/rp1-peripherals.pdf

#define GPIO0_BANKS	3
#define MAX_BANK_PINS	32

#define GPIO0_STATUS(bank, pin)			(ARM_GPIO0_IO_BASE + (bank)*0x4000 + (pin)*8)
	#define STATUS_INISDIRECT__MASK		BIT (16)
	#define STATUS_INFROMPAD__MASK		BIT (17)
	#define STATUS_INFILTERED__MASK		BIT (18)
	#define STATUS_INTOPERI__MASK		BIT (19)
#define GPIO0_CTRL(bank, pin)			(ARM_GPIO0_IO_BASE + (bank)*0x4000 + (pin)*8 + 4)
	#define CTRL_FUNCSEL__SHIFT		0
	#define CTRL_FUNCSEL__MASK		(0x1F << 0)
		#define FUNCSEL_NULL		31
		#define FUNCSEL_GPIO		5
		#define FUNCSEL_ALT_MAX		8
	#define CTRL_FILTER_CONST_M__SHIFT	5
	#define CTRL_FILTER_CONST_M__MASK	(0x7F << 5)
		#define FILTER_CONST_M_MAX	127
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
#define PCIE_INTE(bank)				(ARM_GPIO0_IO_BASE + (bank)*0x4000 + 0x11C)
#define PCIE_INTF(bank)				(ARM_GPIO0_IO_BASE + (bank)*0x4000 + 0x120)
#define PCIE_INTS(bank)				(ARM_GPIO0_IO_BASE + (bank)*0x4000 + 0x124)
#define RIO0_OUT(bank, offset)			(ARM_GPIO0_RIO_BASE + (bank)*0x4000 + (offset) + 0)
	#define RIO_RW_OFFSET			0x0000
	#define RIO_XOR_OFFSET			0x1000
	#define RIO_SET_OFFSET			0x2000
	#define RIO_CLR_OFFSET			0x3000
#define RIO0_OE(bank, offset)			(ARM_GPIO0_RIO_BASE + (bank)*0x4000 + (offset) + 4)
#define RIO0_IN(bank)				(ARM_GPIO0_RIO_BASE + (bank)*0x4000 + 8)
#define PADS0_CTRL(bank, pin)			(ARM_GPIO0_PADS_BASE + (bank)*0x4000 + 4 + (pin) * 4)
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
	CTRL_IRQMASK_EDGE_HIGH__MASK,		// RisingEdge
	CTRL_IRQMASK_EDGE_LOW__MASK,		// FallingEdge
	CTRL_IRQMASK_LEVEL_HIGH__MASK,		// HighLevel
	CTRL_IRQMASK_LEVEL_LOW__MASK,		// LowLevel
	CTRL_IRQMASK_EDGE_HIGH__MASK,		// AsyncRisingEdge
	CTRL_IRQMASK_EDGE_LOW__MASK,		// AsyncFallingEdge
	CTRL_IRQMASK_DB_LEVEL_HIGH__MASK,	// DebouncedHighLevel
	CTRL_IRQMASK_DB_LEVEL_LOW__MASK		// DebouncedLowLevel
};

CGPIOPin::CGPIOPin (void)
:	m_nPin (GPIO_PINS),
	m_nBank (GPIO0_BANKS),
	m_nBankPin (MAX_BANK_PINS),
	m_Mode (GPIOModeUnknown),
	m_pManager (nullptr),
	m_pHandler (nullptr),
	m_Interrupt (GPIOInterruptUnknown),
	m_Interrupt2 (GPIOInterruptUnknown)
{
}

CGPIOPin::CGPIOPin (unsigned nPin, TGPIOMode Mode, CGPIOManager *pManager)
:	m_nPin (GPIO_PINS),
	m_nBank (GPIO0_BANKS),
	m_nBankPin (MAX_BANK_PINS),
	m_Mode (GPIOModeUnknown),
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

	m_nBank = GPIO0_BANKS;
	m_nBankPin = MAX_BANK_PINS;
}

void CGPIOPin::AssignPin (unsigned nPin)
{
	assert (m_nPin == GPIO_PINS);
	assert (m_nBank == GPIO0_BANKS);
	assert (m_nBankPin == MAX_BANK_PINS);

	m_nPin = nPin;

	if (m_nPin >= GPIO_PINS)
	{
		m_nPin = CMachineInfo::Get ()->GetGPIOPin ((TGPIOVirtualPin) nPin);
	}

	assert (m_nPin < GPIO_PINS);

	Pin2Bank (m_nPin, &m_nBank, &m_nBankPin);
}

void CGPIOPin::SetMode (TGPIOMode Mode, boolean bInitPin)
{
	assert (Mode < GPIOModeUnknown);
	assert (m_nBank < GPIO0_BANKS);
	assert (m_nBankPin < MAX_BANK_PINS);
	assert (CSouthbridge::IsInitialized ());

	TGPIOMode PreviousMode = m_Mode;
	m_Mode = Mode;

	if (m_Mode == GPIOModeNone)
	{
		assert (m_nBank < GPIO0_BANKS);
		assert (m_nBankPin < MAX_BANK_PINS);

		uintptr nPadsReg = PADS0_CTRL (m_nBank, m_nBankPin);
		u32 nPadsValue = read32 (nPadsReg);
		nPadsValue |= PADS_OD__MASK;
		nPadsValue &= ~PADS_IE__MASK;
		nPadsValue |= PADS_PDE__MASK;
		nPadsValue &= ~PADS_PUE__MASK;
		write32 (nPadsReg, nPadsValue);

		uintptr nCtrlReg = GPIO0_CTRL (m_nBank, m_nBankPin);
		u32 nCtrlValue = read32 (nCtrlReg);
		nCtrlValue &= ~CTRL_OUTOVER__MASK;
		nCtrlValue |= OUTOVER_FUNCSEL << CTRL_OUTOVER__SHIFT;
		nCtrlValue &= ~CTRL_OEOVER__MASK;
		nCtrlValue |= OEOVER_FUNCSEL << CTRL_OEOVER__SHIFT;
		nCtrlValue &= ~CTRL_FUNCSEL__MASK;
		nCtrlValue |= FUNCSEL_NULL << CTRL_FUNCSEL__SHIFT;
		write32 (nCtrlReg, nCtrlValue);

		return;
	}

	if (GPIOModeAlternateFunction0 <= m_Mode && m_Mode <= GPIOModeAlternateFunction8)
	{
		if (bInitPin)
		{
			SetPullMode (GPIOPullModeOff);
		}

		SetAlternateFunction (m_Mode - GPIOModeAlternateFunction0);

		return;
	}

	if (   bInitPin
	    && m_Mode == GPIOModeOutput)
	{
		SetPullMode (GPIOPullModeOff);
	}

	if (   PreviousMode > GPIOModeInputPullDown
	    || PreviousMode == GPIOModeNone)
	{
		SetAlternateFunction (FUNCSEL_GPIO);
	}

	write32 (RIO0_OE (m_nBank, m_Mode == GPIOModeOutput ? RIO_SET_OFFSET : RIO_CLR_OFFSET),
		 BIT (m_nBankPin));

	DataSyncBarrier ();

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
}

void CGPIOPin::SetPullMode (TGPIOPullMode Mode)
{
	assert (m_nBank < GPIO0_BANKS);
	assert (m_nBankPin < MAX_BANK_PINS);
	assert (CSouthbridge::IsInitialized ());

	u32 nPads = read32 (PADS0_CTRL (m_nBank, m_nBankPin));
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

	write32 (PADS0_CTRL (m_nBank, m_nBankPin), nPads);
}

void CGPIOPin::SetSchmittTrigger (boolean bEnable)
{
	assert (m_nBank < GPIO0_BANKS);
	assert (m_nBankPin < MAX_BANK_PINS);

	uintptr nPadsReg = PADS0_CTRL (m_nBank, m_nBankPin);
	u32 nPadsValue = read32 (nPadsReg);

	if (bEnable)
	{
		nPadsValue |= PADS_SCHMITT__MASK;
	}
	else
	{
		nPadsValue &= ~PADS_SCHMITT__MASK;
	}

	write32 (nPadsReg, nPadsValue);
}

void CGPIOPin::SetFilterConstant (unsigned nFilterConstant)
{
	assert (m_nBank < GPIO0_BANKS);
	assert (m_nBankPin < MAX_BANK_PINS);
	assert (nFilterConstant <= FILTER_CONST_M_MAX);

	uintptr nCtrlReg = GPIO0_CTRL (m_nBank, m_nBankPin);
	u32 nCtrlValue = read32 (nCtrlReg);

	nCtrlValue &= ~CTRL_FILTER_CONST_M__MASK;
	nCtrlValue |= nFilterConstant << CTRL_FILTER_CONST_M__SHIFT;

	write32 (nCtrlReg, nCtrlValue);
}

void CGPIOPin::SetDriveStrength (TGPIODriveStrength DriveStrength)
{
	assert (m_nBank < GPIO0_BANKS);
	assert (m_nBankPin < MAX_BANK_PINS);
	assert (DriveStrength < GPIODriveStrengthUnknown);

	uintptr nPadsReg = PADS0_CTRL (m_nBank, m_nBankPin);
	u32 nPadsValue = read32 (nPadsReg);

	nPadsValue &= ~PADS_DRIVE__MASK;
	nPadsValue |= DriveStrength << PADS_DRIVE__SHIFT;

	write32 (nPadsReg, nPadsValue);
}

void CGPIOPin::SetSlewRate (TGPIOSlewRate SlewRate)
{
	assert (m_nBank < GPIO0_BANKS);
	assert (m_nBankPin < MAX_BANK_PINS);
	assert (SlewRate < GPIOSlewRateUnknown);

	uintptr nPadsReg = PADS0_CTRL (m_nBank, m_nBankPin);
	u32 nPadsValue = read32 (nPadsReg);

	if (SlewRate == GPIOSlewRateSlow)
	{
		nPadsValue &= ~PADS_SLEWFAST__MASK;
	}
	else
	{
		nPadsValue |= PADS_SLEWFAST__MASK;
	}

	write32 (nPadsReg, nPadsValue);
}

void CGPIOPin::Write (unsigned nValue)
{
	assert (m_nBank < GPIO0_BANKS);
	assert (m_nBankPin < MAX_BANK_PINS);
	assert (nValue <= HIGH);

	write32 (RIO0_OUT (m_nBank, nValue ? RIO_SET_OFFSET : RIO_CLR_OFFSET), BIT (m_nBankPin));

	DataSyncBarrier ();
}

unsigned CGPIOPin::Read (void) const
{
	assert (m_nBank < GPIO0_BANKS);
	assert (m_nBankPin < MAX_BANK_PINS);
	assert (   m_Mode == GPIOModeInput
		|| m_Mode == GPIOModeInputPullUp
		|| m_Mode == GPIOModeInputPullDown);

	return !!(read32 (RIO0_IN (m_nBank)) & BIT (m_nBankPin));
}

void CGPIOPin::Invert (void)
{
	assert (m_nBank < GPIO0_BANKS);
	assert (m_nBankPin < MAX_BANK_PINS);

	write32 (RIO0_OUT (m_nBank, RIO_XOR_OFFSET), BIT (m_nBankPin));

	DataSyncBarrier ();
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

	assert (m_nBank < GPIO0_BANKS);
	assert (m_nBankPin < MAX_BANK_PINS);
	s_SpinLock.Acquire ();
	write32 (PCIE_INTE (m_nBank), read32 (PCIE_INTE (m_nBank)) | BIT (m_nBankPin));
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

	assert (m_nBank < GPIO0_BANKS);
	assert (m_nBankPin < MAX_BANK_PINS);
	s_SpinLock.Acquire ();
	write32 (PCIE_INTE (m_nBank), read32 (PCIE_INTE (m_nBank)) & ~BIT (m_nBankPin));
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

	assert (m_nBank < GPIO0_BANKS);
	assert (m_nBankPin < MAX_BANK_PINS);
	uintptr nReg = GPIO0_CTRL (m_nBank, m_nBankPin);

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

	assert (m_nBank < GPIO0_BANKS);
	assert (m_nBankPin < MAX_BANK_PINS);
	uintptr nReg = GPIO0_CTRL (m_nBank, m_nBankPin);

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

	assert (m_nBank < GPIO0_BANKS);
	assert (m_nBankPin < MAX_BANK_PINS);
	uintptr nReg = GPIO0_CTRL (m_nBank, m_nBankPin);

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

	assert (m_nBank < GPIO0_BANKS);
	assert (m_nBankPin < MAX_BANK_PINS);
	uintptr nReg = GPIO0_CTRL (m_nBank, m_nBankPin);

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

	assert (m_nBank < GPIO0_BANKS);
	assert (m_nBankPin < MAX_BANK_PINS);
	uintptr nReg = GPIO0_CTRL (m_nBank, m_nBankPin);

	m_SpinLock.Acquire ();
	write32 (nReg, read32 (nReg) | CTRL_IRQRESET__MASK);
	// assert (!(read32 (nReg) & CTRL_IRQRESET__MASK));
	m_SpinLock.Release ();
}

void CGPIOPin::SetModeAll (u32 nInputMask, u32 nOutputMask)
{
	assert (!(nInputMask & nOutputMask));

	if (nInputMask)
	{
		write32 (RIO0_OE (0, RIO_CLR_OFFSET), nInputMask);
	}

	if (nOutputMask)
	{
		write32 (RIO0_OE (0, RIO_SET_OFFSET), nOutputMask);
	}

	DataSyncBarrier ();
}

void CGPIOPin::WriteAll (u32 nValue, u32 nMask)
{
	u32 nClear = ~nValue & nMask;
	if (nClear)
	{
		write32 (RIO0_OUT (0, RIO_CLR_OFFSET), nClear);
	}

	u32 nSet = nValue & nMask;
	if (nSet)
	{
		write32 (RIO0_OUT (0, RIO_SET_OFFSET), nSet);
	}

	DataSyncBarrier ();
}

u32 CGPIOPin::ReadAll (void)
{
	return read32 (RIO0_IN (0));
}

#ifndef NDEBUG

void CGPIOPin::DumpStatus (void)
{
	if (CSouthbridge::IsInitialized ())
	{
		LOGDBG ("GIO STATUS   CTRL     PADS");

		for (unsigned nPin = 0; nPin < GPIO_PINS; nPin++)
		{
			unsigned nBank;
			unsigned nBankPin;
			Pin2Bank (nPin, &nBank, &nBankPin);

			LOGDBG ("%02u: %08X %08X %02X",
				nPin,
				read32 (GPIO0_STATUS (nBank, nBankPin)),
				read32 (GPIO0_CTRL (nBank, nBankPin)),
				read32 (PADS0_CTRL (nBank, nBankPin)));
		}
	}
	else
	{
		LOGDBG ("RP1 is not initialized");
	}
}

#endif

void CGPIOPin::SetAlternateFunction (unsigned nFunction)
{
	assert (nFunction <= FUNCSEL_ALT_MAX);
	assert (m_nBank < GPIO0_BANKS);
	assert (m_nBankPin < MAX_BANK_PINS);

	uintptr nPadsReg = PADS0_CTRL (m_nBank, m_nBankPin);
	u32 nPadsValue = read32 (nPadsReg);
	nPadsValue &= ~PADS_OD__MASK;
	nPadsValue |= PADS_IE__MASK;
	write32 (nPadsReg, nPadsValue);

	uintptr nCtrlReg = GPIO0_CTRL (m_nBank, m_nBankPin);
	u32 nCtrlValue = read32 (nCtrlReg);
	nCtrlValue &= ~CTRL_INOVER__MASK;
	nCtrlValue |= INOVER_DONT_INVERT << CTRL_INOVER__SHIFT;
	nCtrlValue &= ~CTRL_OUTOVER__MASK;
	nCtrlValue |= OUTOVER_FUNCSEL << CTRL_OUTOVER__SHIFT;
	nCtrlValue &= ~CTRL_OEOVER__MASK;
	nCtrlValue |= OEOVER_FUNCSEL << CTRL_OEOVER__SHIFT;
	nCtrlValue &= ~CTRL_FUNCSEL__MASK;
	nCtrlValue |= nFunction << CTRL_FUNCSEL__SHIFT;
	write32 (nCtrlReg, nCtrlValue);
}

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

	unsigned nBank;
	unsigned nBankPin;
	Pin2Bank (nPin, &nBank, &nBankPin);

	s_SpinLock.Acquire ();
	write32 (PCIE_INTE (nBank), read32 (PCIE_INTE (nBank)) & ~BIT (nBankPin));
	s_SpinLock.Release ();
}

void CGPIOPin::Pin2Bank (unsigned nPin, unsigned *pBank, unsigned *pBankPin)
{
	assert (pBank);
	assert (pBankPin);

	if (nPin < 28)
	{
		*pBank = 0;
		*pBankPin = nPin;
	}
	else if (nPin < 34)
	{
		*pBank = 1;
		*pBankPin = nPin - 28;
	}
	else
	{
		assert (nPin < 54);
		*pBank = 2;
		*pBankPin = nPin - 34;
	}
}
