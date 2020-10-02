//
// kernel.cpp
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
#include "kernel.h"
#include <circle/usb/usb.h>
#include <circle/synchronize.h>
#include <circle/debug.h>

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_USBHCI (&m_Interrupt, &m_Timer)
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
		bOK = m_Serial.Initialize (115200);
	}

	if (bOK)
	{
		CDevice *pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
		if (pTarget == 0)
		{
			pTarget = &m_Screen;
		}

		bOK = m_Logger.Initialize (pTarget);
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
		bOK = m_USBHCI.Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	CUSBFunction *pUSBHub1 = (CUSBFunction *) m_DeviceNameService.GetDevice ("uhub1", FALSE);
	if (pUSBHub1 == 0)
	{
		m_Logger.Write (FromKernel, LogPanic, "USB hub not found");
	}

	DMA_BUFFER (u8, DeviceDescBuffer, sizeof (TUSBDeviceDescriptor));
	TUSBDeviceDescriptor *pDeviceDescriptor = (TUSBDeviceDescriptor *) DeviceDescBuffer;
	if (m_USBHCI.GetDescriptor (pUSBHub1->GetDevice ()->GetEndpoint0 (),
				   DESCRIPTOR_DEVICE, DESCRIPTOR_INDEX_DEFAULT,
				   pDeviceDescriptor, sizeof *pDeviceDescriptor)
	    == sizeof *pDeviceDescriptor)
	{
		m_Logger.Write (FromKernel, LogNotice,
				"USB hub: Vendor 0x%04X, Product 0x%04X",
				(unsigned) pDeviceDescriptor->idVendor,
				(unsigned) pDeviceDescriptor->idProduct);
#ifndef NDEBUG
		m_Logger.Write (FromKernel, LogNotice, "Dumping device descriptor");

		debug_hexdump (pDeviceDescriptor, sizeof *pDeviceDescriptor, FromKernel);
#endif
	}
	else
	{
		m_Logger.Write (FromKernel, LogError, "Cannot get device descriptor");
	}

	return ShutdownHalt;
}
