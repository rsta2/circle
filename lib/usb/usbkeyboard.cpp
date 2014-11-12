//
// usbkeyboard.cpp
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
#include <circle/usb/usbkeyboard.h>
#include <circle/usb/usbhid.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/devicenameservice.h>
#include <circle/logger.h>
#include <circle/timer.h>
#include <assert.h>

//#define REPEAT_ENABLE					// does not work well with any Keyboard

#define REPEAT_DELAY		MSEC2HZ (400)
#define REPEAT_RATE		MSEC2HZ (80)

unsigned CUSBKeyboardDevice::s_nDeviceNumber = 1;

static const char FromUSBKbd[] = "usbkbd";

CUSBKeyboardDevice::CUSBKeyboardDevice (CUSBDevice *pDevice)
:	CUSBDevice (pDevice),
	m_pReportEndpoint (0),
	m_pKeyPressedHandler (0),
	m_pSelectConsoleHandler (0),
	m_pShutdownHandler (0),
	m_pKeyStatusHandlerRaw (0),
	m_pURB (0),
	m_pReportBuffer (0),
	m_ucLastPhyCode (0),
	m_hTimer (0)
{
	m_pReportBuffer = new u8[BOOT_REPORT_SIZE];
	assert (m_pReportBuffer != 0);
}

CUSBKeyboardDevice::~CUSBKeyboardDevice (void)
{
	delete [] m_pReportBuffer;
	m_pReportBuffer = 0;
	
	delete m_pReportEndpoint;
	m_pReportEndpoint = 0;
}

boolean CUSBKeyboardDevice::Configure (void)
{
	TUSBConfigurationDescriptor *pConfDesc =
		(TUSBConfigurationDescriptor *) GetDescriptor (DESCRIPTOR_CONFIGURATION);
	if (   pConfDesc == 0
	    || pConfDesc->bNumInterfaces <  1)
	{
		ConfigurationError (FromUSBKbd);

		return FALSE;
	}

	TUSBInterfaceDescriptor *pInterfaceDesc;
	while ((pInterfaceDesc = (TUSBInterfaceDescriptor *) GetDescriptor (DESCRIPTOR_INTERFACE)) != 0)
	{
		if (   pInterfaceDesc->bNumEndpoints	  <  1
		    || pInterfaceDesc->bInterfaceClass	  != 0x03	// HID Class
		    || pInterfaceDesc->bInterfaceSubClass != 0x01	// Boot Interface Subclass
		    || pInterfaceDesc->bInterfaceProtocol != 0x01)	// Keyboard
		{
			continue;
		}

		m_ucInterfaceNumber  = pInterfaceDesc->bInterfaceNumber;
		m_ucAlternateSetting = pInterfaceDesc->bAlternateSetting;

		TUSBEndpointDescriptor *pEndpointDesc =
			(TUSBEndpointDescriptor *) GetDescriptor (DESCRIPTOR_ENDPOINT);
		if (   pEndpointDesc == 0
		    || (pEndpointDesc->bEndpointAddress & 0x80) != 0x80		// Input EP
		    || (pEndpointDesc->bmAttributes     & 0x3F)	!= 0x03)	// Interrupt EP
		{
			continue;
		}

		assert (m_pReportEndpoint == 0);
		m_pReportEndpoint = new CUSBEndpoint (this, pEndpointDesc);
		assert (m_pReportEndpoint != 0);

		break;
	}

	if (m_pReportEndpoint == 0)
	{
		ConfigurationError (FromUSBKbd);

		return FALSE;
	}
	
	if (!CUSBDevice::Configure ())
	{
		CLogger::Get ()->Write (FromUSBKbd, LogError, "Cannot set configuration");

		return FALSE;
	}

	if (GetHost ()->ControlMessage (GetEndpoint0 (),
					REQUEST_OUT | REQUEST_TO_INTERFACE, SET_INTERFACE,
					m_ucAlternateSetting,
					m_ucInterfaceNumber, 0, 0) < 0)
	{
		CLogger::Get ()->Write (FromUSBKbd, LogError, "Cannot set interface");

		return FALSE;
	}

	if (GetHost ()->ControlMessage (GetEndpoint0 (),
					REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_INTERFACE,
					SET_PROTOCOL, BOOT_PROTOCOL,
					m_ucInterfaceNumber, 0, 0) < 0)
	{
		CLogger::Get ()->Write (FromUSBKbd, LogError, "Cannot set boot protocol");

		return FALSE;
	}

	CString DeviceName;
	DeviceName.Format ("ukbd%u", s_nDeviceNumber++);
	CDeviceNameService::Get ()->AddDevice (DeviceName, this, FALSE);

	return StartRequest ();
}

void CUSBKeyboardDevice::RegisterKeyPressedHandler (TKeyPressedHandler *pKeyPressedHandler)
{
	assert (pKeyPressedHandler != 0);
	m_pKeyPressedHandler = pKeyPressedHandler;
}

void CUSBKeyboardDevice::RegisterSelectConsoleHandler (TSelectConsoleHandler *pSelectConsoleHandler)
{
	assert (pSelectConsoleHandler != 0);
	m_pSelectConsoleHandler = pSelectConsoleHandler;
}

void CUSBKeyboardDevice::RegisterShutdownHandler (TShutdownHandler *pShutdownHandler)
{
	assert (pShutdownHandler != 0);
	m_pShutdownHandler = pShutdownHandler;
}

void CUSBKeyboardDevice::RegisterKeyStatusHandlerRaw (TKeyStatusHandlerRaw *pKeyStatusHandlerRaw)
{
	assert (pKeyStatusHandlerRaw != 0);
	m_pKeyStatusHandlerRaw = pKeyStatusHandlerRaw;
}

void CUSBKeyboardDevice::GenerateKeyEvent (u8 ucPhyCode)
{
	const char *pKeyString;
	char Buffer[2];

	u8 ucModifiers = GetModifiers ();
	u8 ucLogCode = m_KeyMap.Translate (ucPhyCode, ucModifiers);

	switch (ucLogCode)
	{
	case ActionSwitchCapsLock:
	case ActionSwitchNumLock:
	case ActionSwitchScrollLock:
		break;

	case ActionSelectConsole1:
	case ActionSelectConsole2:
	case ActionSelectConsole3:
	case ActionSelectConsole4:
	case ActionSelectConsole5:
	case ActionSelectConsole6:
	case ActionSelectConsole7:
	case ActionSelectConsole8:
	case ActionSelectConsole9:
	case ActionSelectConsole10:
	case ActionSelectConsole11:
	case ActionSelectConsole12:
		if (m_pSelectConsoleHandler != 0)
		{
			unsigned nConsole = ucLogCode - ActionSelectConsole1;
			assert (nConsole < 12);

			(*m_pSelectConsoleHandler) (nConsole);
		}
		break;

	case ActionShutdown:
		if (m_pShutdownHandler != 0)
		{
			(*m_pShutdownHandler) ();
		}
		break;

	default:
		pKeyString = m_KeyMap.GetString (ucLogCode, ucModifiers, Buffer);
		if (pKeyString != 0)
		{
			if (m_pKeyPressedHandler != 0)
			{
				(*m_pKeyPressedHandler) (pKeyString);
			}
		}
		break;
	}
}

boolean CUSBKeyboardDevice::StartRequest (void)
{
	assert (m_pReportEndpoint != 0);
	assert (m_pReportBuffer != 0);
	
	assert (m_pURB == 0);
	m_pURB = new CUSBRequest (m_pReportEndpoint, m_pReportBuffer, BOOT_REPORT_SIZE);
	assert (m_pURB != 0);
	m_pURB->SetCompletionRoutine (CompletionStub, 0, this);
	
	return GetHost ()->SubmitAsyncRequest (m_pURB);
}

void CUSBKeyboardDevice::CompletionRoutine (CUSBRequest *pURB)
{
	assert (pURB != 0);
	assert (m_pURB == pURB);

	if (   pURB->GetStatus () != 0
	    && pURB->GetResultLength () == BOOT_REPORT_SIZE)
	{
		if (m_pKeyStatusHandlerRaw != 0)
		{
			(*m_pKeyStatusHandlerRaw) (GetModifiers (), m_pReportBuffer+2);
		}
		else
		{
			u8 ucPhyCode = GetKeyCode ();

			if (ucPhyCode == m_ucLastPhyCode)
			{
				ucPhyCode = 0;
			}
			else
			{
				m_ucLastPhyCode = ucPhyCode;
			}
			
			if (ucPhyCode != 0)
			{
				GenerateKeyEvent (ucPhyCode);
#ifdef REPEAT_ENABLE
				if (m_hTimer != 0)
				{
					CTimer::Get ()->CancelKernelTimer (m_hTimer);
				}

				m_hTimer = CTimer::Get ()->StartKernelTimer (REPEAT_DELAY, TimerStub, 0, this);
				assert (m_hTimer != 0);
#endif
			}
			else if (m_hTimer != 0)
			{
				CTimer::Get ()->CancelKernelTimer (m_hTimer);
				m_hTimer = 0;
			}
		}
	}

	delete m_pURB;
	m_pURB = 0;
	
	StartRequest ();
}

void CUSBKeyboardDevice::CompletionStub (CUSBRequest *pURB, void *pParam, void *pContext)
{
	CUSBKeyboardDevice *pThis = (CUSBKeyboardDevice *) pContext;
	assert (pThis != 0);
	
	pThis->CompletionRoutine (pURB);
}

u8 CUSBKeyboardDevice::GetModifiers (void) const
{
	return m_pReportBuffer[0];
}

u8 CUSBKeyboardDevice::GetKeyCode (void) const
{
	for (unsigned i = 7; i >= 2; i--)
	{
		u8 ucKeyCode = m_pReportBuffer[i];
		if (ucKeyCode != 0)
		{
			return ucKeyCode;
		}
	}
	
	return 0;
}

void CUSBKeyboardDevice::TimerHandler (unsigned hTimer)
{
	assert (hTimer == m_hTimer);

	if (m_ucLastPhyCode != 0)
	{
		GenerateKeyEvent (m_ucLastPhyCode);

		m_hTimer = CTimer::Get ()->StartKernelTimer (REPEAT_RATE, TimerStub, 0, this);
		assert (m_hTimer != 0);
	}
}

void CUSBKeyboardDevice::TimerStub (unsigned hTimer, void *pParam, void *pContext)
{
	CUSBKeyboardDevice *pThis = (CUSBKeyboardDevice *) pContext;
	assert (pThis != 0);

	pThis->TimerHandler (hTimer);
}
