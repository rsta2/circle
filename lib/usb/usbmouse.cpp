//
// usbmouse.cpp
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
#include <circle/usb/usbmouse.h>
#include <circle/usb/usbhid.h>
#include <circle/devicenameservice.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <assert.h>

#define REPORT_SIZE	3

unsigned CUSBMouseDevice::s_nDeviceNumber = 1;

static const char FromUSBMouse[] = "umouse";

CUSBMouseDevice::CUSBMouseDevice (CUSBFunction *pFunction)
:	CUSBHIDDevice (pFunction, REPORT_SIZE),
	m_pStatusHandler (0)
{
}

CUSBMouseDevice::~CUSBMouseDevice (void)
{
	m_pStatusHandler = 0;
}

boolean CUSBMouseDevice::Configure (void)
{
	if (!CUSBHIDDevice::Configure ())
	{
		CLogger::Get ()->Write (FromUSBMouse, LogError, "Cannot configure HID device");

		return FALSE;
	}

	CString DeviceName;
	DeviceName.Format ("umouse%u", s_nDeviceNumber++);
	CDeviceNameService::Get ()->AddDevice (DeviceName, this, FALSE);

	return TRUE;
}

void CUSBMouseDevice::RegisterStatusHandler (TMouseStatusHandler *pStatusHandler)
{
	assert (m_pStatusHandler == 0);
	m_pStatusHandler = pStatusHandler;
	assert (m_pStatusHandler != 0);
}

void CUSBMouseDevice::ReportHandler (const u8 *pReport)
{
	if (   pReport != 0
	    && m_pStatusHandler != 0)
	{
		u8 ucHIDButtons = pReport[0];

		unsigned nButtons = 0;
		if (ucHIDButtons & USBHID_BUTTON1)
		{
			nButtons |= MOUSE_BUTTON_LEFT;
		}
		if (ucHIDButtons & USBHID_BUTTON2)
		{
			nButtons |= MOUSE_BUTTON_RIGHT;
		}
		if (ucHIDButtons & USBHID_BUTTON3)
		{
			nButtons |= MOUSE_BUTTON_MIDDLE;
		}

		(*m_pStatusHandler) (nButtons, char2int ((char) pReport[1]), char2int ((char) pReport[2]));
	}
}
