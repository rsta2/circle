//
// usbkeyboard.cpp
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
#include <circle/usb/usbkeyboard.h>
#include <circle/devicenameservice.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/synchronize.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <circle/macros.h>
#include <assert.h>

CNumberPool CUSBKeyboardDevice::s_DeviceNumberPool (1);

static const char FromUSBKbd[] = "usbkbd";
static const char DevicePrefix[] = "ukbd";

CUSBKeyboardDevice::CUSBKeyboardDevice (CUSBFunction *pFunction)
:	CUSBHIDDevice (pFunction, USBKEYB_REPORT_SIZE),
	m_pKeyStatusHandlerRaw (0),
	m_pKeyStatusHandlerRawArg (0),
	m_bMixedMode (FALSE),
	m_ucLastLEDStatus (0),
	m_nDeviceNumber (0)		// not assigned
{
	memset (m_LastReport, 0, sizeof m_LastReport);
}

CUSBKeyboardDevice::~CUSBKeyboardDevice (void)
{
	m_pKeyStatusHandlerRaw = 0;

	if (m_nDeviceNumber != 0)
	{
		CDeviceNameService::Get ()->RemoveDevice (DevicePrefix, m_nDeviceNumber, FALSE);

		s_DeviceNumberPool.FreeNumber (m_nDeviceNumber);
	}
}

boolean CUSBKeyboardDevice::Configure (void)
{
	if (!CUSBHIDDevice::ConfigureHID ())
	{
		CLogger::Get ()->Write (FromUSBKbd, LogError, "Cannot configure HID device");

		return FALSE;
	}

	// setting the LED status forces some keyboard adapters to work
	SetLEDs (m_ucLastLEDStatus);

	m_nDeviceNumber = s_DeviceNumberPool.AllocateNumber (TRUE, FromUSBKbd);

	CDeviceNameService::Get ()->AddDevice (DevicePrefix, m_nDeviceNumber, this, FALSE);

	return StartRequest ();
}

void CUSBKeyboardDevice::RegisterKeyPressedHandler (TKeyPressedHandler *pKeyPressedHandler)
{
	m_Behaviour.RegisterKeyPressedHandler (pKeyPressedHandler);
}

void CUSBKeyboardDevice::RegisterSelectConsoleHandler (TSelectConsoleHandler *pSelectConsoleHandler)
{
	m_Behaviour.RegisterSelectConsoleHandler (pSelectConsoleHandler);
}

void CUSBKeyboardDevice::RegisterShutdownHandler (TShutdownHandler *pShutdownHandler)
{
	m_Behaviour.RegisterShutdownHandler (pShutdownHandler);
}

void CUSBKeyboardDevice::UpdateLEDs (void)
{
	if (   m_pKeyStatusHandlerRaw == 0
	    || m_bMixedMode)
	{
		u8 ucLEDStatus = GetLEDStatus ();
		if (ucLEDStatus != m_ucLastLEDStatus)
		{
			m_ucLastLEDStatus = ucLEDStatus;
			if (!SetLEDs (m_ucLastLEDStatus))
			{
				CLogger::Get ()->Write (FromUSBKbd, LogError, "Cannot set LED status");
			}
		}
	}
}

u8 CUSBKeyboardDevice::GetLEDStatus (void) const
{
	u8 ucStatus = m_Behaviour.GetLEDStatus ();

	u8 ucResult = 0;

	if (ucStatus & KEYB_LED_NUM_LOCK)
	{
		ucResult |= LED_NUM_LOCK;
	}

	if (ucStatus & KEYB_LED_CAPS_LOCK)
	{
		ucResult |= LED_CAPS_LOCK;
	}

	if (ucStatus & KEYB_LED_SCROLL_LOCK)
	{
		ucResult |= LED_SCROLL_LOCK;
	}

	return ucResult;
}

void CUSBKeyboardDevice::RegisterKeyStatusHandlerRaw (TKeyStatusHandlerRawEx *pKeyStatusHandlerRaw,
					boolean bMixedMode, void* pArg)
{
	assert (pKeyStatusHandlerRaw != 0);
	m_pKeyStatusHandlerRaw = pKeyStatusHandlerRaw;
	m_pKeyStatusHandlerRawArg = pArg;
	m_bMixedMode = bMixedMode;
}

static void proxy_handler(unsigned char ucModifiers, const unsigned char RawKeys[6], void* arg)
{
	((TKeyStatusHandlerRaw*)arg)(ucModifiers, RawKeys);
}

void CUSBKeyboardDevice::RegisterKeyStatusHandlerRaw (TKeyStatusHandlerRaw *pKeyStatusHandlerRaw,
						      boolean bMixedMode)
{
	RegisterKeyStatusHandlerRaw(proxy_handler, bMixedMode, (void*)pKeyStatusHandlerRaw);
}

void CUSBKeyboardDevice::UnregisterKeyStatusHandlerRaw (void)
{
	m_pKeyStatusHandlerRaw = 0;
	m_pKeyStatusHandlerRawArg = 0;
}

boolean CUSBKeyboardDevice::SetLEDs (u8 ucStatus)
{
	DMA_BUFFER (u8, Buffer, 1) = {ucStatus};

	if (GetHost ()->ControlMessage (GetEndpoint0 (),
					REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_INTERFACE,
					SET_REPORT, REPORT_TYPE_OUTPUT << 8,
					GetInterfaceNumber (), Buffer, 1) < 0)
	{
		return FALSE;
	}

	return TRUE;
}

void CUSBKeyboardDevice::ReportHandler (const u8 *pReport, unsigned nReportSize)
{
	if (   pReport == 0
	    || nReportSize != USBKEYB_REPORT_SIZE)
	{
		return;
	}

	if (m_pKeyStatusHandlerRaw != 0)
	{
		(*m_pKeyStatusHandlerRaw) (pReport[0], pReport+2, m_pKeyStatusHandlerRawArg);

		if (!m_bMixedMode)
		{
			return;
		}
	}

	// report modifier keys
	for (unsigned i = 0; i < 8; i++)
	{
		unsigned nMask = 1 << i;

		if (    (pReport[0] & nMask)
		    && !(m_LastReport[0] & nMask))
		{
			m_Behaviour.KeyPressed (0x80 + i);
		}
		else if (   !(pReport[0] & nMask)
			 &&  (m_LastReport[0] & nMask))
		{
			m_Behaviour.KeyReleased (0x80 + i);
		}
	}

	// report released keys
	for (unsigned i = 2; i < USBKEYB_REPORT_SIZE; i++)
	{
		u8 ucKeyCode = m_LastReport[i];
		if (   ucKeyCode != 0
		    && !FindByte (pReport+2, ucKeyCode, USBKEYB_REPORT_SIZE-2))
		{
			m_Behaviour.KeyReleased (ucKeyCode);
		}
	}

	// report pressed keys
	for (unsigned i = 2; i < USBKEYB_REPORT_SIZE; i++)
	{
		u8 ucKeyCode = pReport[i];
		if (   ucKeyCode != 0
		    && !FindByte (m_LastReport+2, ucKeyCode, USBKEYB_REPORT_SIZE-2))
		{
			m_Behaviour.KeyPressed (ucKeyCode);
		}
	}

	memcpy (m_LastReport, pReport, sizeof m_LastReport);
}

boolean CUSBKeyboardDevice::FindByte (const u8 *pBuffer, u8 ucByte, unsigned nLength)
{
	while (nLength-- > 0)
	{
		if (*pBuffer++ == ucByte)
		{
			return TRUE;
		}
	}

	return FALSE;
}
