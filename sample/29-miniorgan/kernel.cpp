//
// kernel.cpp
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
#include "kernel.h"
#include <circle/usb/usbhcidevice.h>
#include <circle/usb/gadget/usbmidigadget.h>
#include <circle/machineinfo.h>
#include <assert.h>

//#define USB_GADGET_MODE

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_I2CMaster (CMachineInfo::Get ()->GetDevice (DeviceI2CMaster), TRUE),
#ifndef USB_GADGET_MODE
	m_pUSB (new CUSBHCIDevice (&m_Interrupt, &m_Timer, TRUE)), // TRUE: enable plug-and-play
#else
	m_pUSB (new CUSBMIDIGadget (&m_Interrupt)),
#endif
	m_pMiniOrgan (0)
{
	m_ActLED.Blink (5);	// show we are alive
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;

	if (bOK)
	{
		bOK = m_Screen.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Logger.Initialize (&m_Screen);
	}

	if (bOK)
	{
		bOK = m_Interrupt.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Timer.Initialize ();
	}

	if (bOK)
	{
		bOK = m_I2CMaster.Initialize ();
	}

	if (bOK)
	{
		assert (m_pUSB);
		bOK = m_pUSB->Initialize ();
	}

	if (bOK)
	{
		m_pMiniOrgan = new CMiniOrgan (&m_Interrupt, &m_I2CMaster);

		bOK = m_pMiniOrgan->Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);
	
	m_Logger.Write (FromKernel, LogNotice, "Just play!");

	m_pMiniOrgan->Start ();

	for (unsigned nCount = 0; m_pMiniOrgan->IsActive (); nCount++)
	{
		// This must be called from TASK_LEVEL to update the tree of connected USB devices.
		assert (m_pUSB);
		boolean bUpdated = m_pUSB->UpdatePlugAndPlay ();

		m_pMiniOrgan->Process (bUpdated);

		m_Screen.Rotor (0, nCount);
	}

	return ShutdownHalt;
}
