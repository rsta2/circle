//
// usbtouchscreen.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/usbtouchscreen.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/logger.h>
#include <assert.h>

static const char From[] = "utouch";

CUSBTouchScreenDevice::CUSBTouchScreenDevice (CUSBFunction *pFunction)
:	CUSBHIDDevice (pFunction, USBTOUCH_REPORT_SIZE),
	m_bFingerIsDown (FALSE),
	m_pDevice (0)
{
}

CUSBTouchScreenDevice::~CUSBTouchScreenDevice (void)
{
	delete m_pDevice;
	m_pDevice = 0;
}

boolean CUSBTouchScreenDevice::Configure (void)
{
	if (!CUSBHIDDevice::Configure ())
	{
		CLogger::Get ()->Write (From, LogError, "Cannot configure HID device");

		return FALSE;
	}

	if (!StartRequest ())
	{
		return FALSE;
	}

	assert (m_pDevice == 0);
	m_pDevice = new CTouchScreenDevice;
	assert (m_pDevice != 0);

	return TRUE;
}

void CUSBTouchScreenDevice::ReportHandler (const u8 *pReport, unsigned nReportSize)
{
	if (   pReport == 0
	    || nReportSize != USBTOUCH_REPORT_SIZE
	    || pReport[0] != 1		// Report ID
	    || pReport[2] != 1)		// Contact Identifier
	{
		return;
	}

	assert (m_pDevice != 0);

	if (   pReport[1] == 1		// Tip Switch
	    && pReport[3] > 0)		// Tip Pressure
	{
		u16 x = pReport[4] | (u16) pReport[5] << 8;
		u16 y = pReport[6] | (u16) pReport[7] << 8;

		if (!m_bFingerIsDown)
		{
			m_pDevice->ReportHandler (TouchScreenEventFingerDown, 0, x, y);
		}
		else if (   m_usLastX != x
		         && m_usLastY != y)
		{
			m_pDevice->ReportHandler (TouchScreenEventFingerMove, 0, x, y);
		}

		m_bFingerIsDown = TRUE;
		m_usLastX = x;
		m_usLastY = y;
	}
	else if (   pReport[1] == 0	// Tip Switch
		 || pReport[3] == 0)	// Tip Pressure
	{
		if (m_bFingerIsDown)
		{
			m_pDevice->ReportHandler (TouchScreenEventFingerUp, 0, 0, 0);
		}

		m_bFingerIsDown = FALSE;
	}
}
