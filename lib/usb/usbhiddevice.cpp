//
// usbhiddevice.cpp
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
#include <circle/usb/usbhiddevice.h>
#include <circle/usb/usbhid.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <assert.h>

static const char FromUSBHID[] = "usbhid";

CUSBHIDDevice::CUSBHIDDevice (CUSBFunction *pFunction, unsigned nMaxReportSize)
:	CUSBFunction (pFunction),
	m_nMaxReportSize (nMaxReportSize),
	m_pReportEndpoint (0),
	m_pEndpointOut (0),
	m_pReportBuffer (0)
{
	if (m_nMaxReportSize > 0)
	{
		m_pReportBuffer = new u8[m_nMaxReportSize];
		assert (m_pReportBuffer != 0);
	}
}

CUSBHIDDevice::~CUSBHIDDevice (void)
{
	delete [] m_pReportBuffer;
	m_pReportBuffer = 0;

	delete m_pEndpointOut;
	m_pEndpointOut = 0;

	delete m_pReportEndpoint;
	m_pReportEndpoint = 0;
}

boolean CUSBHIDDevice::ConfigureHID (unsigned nMaxReportSize)
{
	if (GetNumEndpoints () < 1)
	{
		ConfigurationError (FromUSBHID);

		return FALSE;
	}

	const TUSBEndpointDescriptor *pEndpointDesc;
	while ((pEndpointDesc = (TUSBEndpointDescriptor *) GetDescriptor (DESCRIPTOR_ENDPOINT)) != 0)
	{
		if ((pEndpointDesc->bmAttributes & 0x3F) == 0x03)		// Interrupt EP
		{
			if ((pEndpointDesc->bEndpointAddress & 0x80) == 0x80)	// Input EP
			{
				if (m_pReportEndpoint != 0)
				{
					ConfigurationError (FromUSBHID);

					return FALSE;
				}

				m_pReportEndpoint = new CUSBEndpoint (GetDevice (), pEndpointDesc);
			}
			else							// Output EP
			{
				if (m_pEndpointOut != 0)
				{
					ConfigurationError (FromUSBHID);

					return FALSE;
				}

				m_pEndpointOut = new CUSBEndpoint (GetDevice (), pEndpointDesc);
			}
		}
	}

	if (m_pReportEndpoint == 0)
	{
		ConfigurationError (FromUSBHID);

		return FALSE;
	}

	if (!CUSBFunction::Configure ())
	{
		CLogger::Get ()->Write (FromUSBHID, LogError, "Cannot set interface");

		return FALSE;
	}

	if (   GetInterfaceClass ()    == 3	// HID class
	    && GetInterfaceSubClass () == 1)	// boot class
	{
		if (GetHost ()->ControlMessage (GetEndpoint0 (),
						REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_INTERFACE,
						SET_PROTOCOL,   GetInterfaceProtocol () == 2
							      ? REPORT_PROTOCOL : BOOT_PROTOCOL,
						GetInterfaceNumber (), 0, 0) < 0)
		{
			CLogger::Get ()->Write (FromUSBHID, LogError, "Cannot set protocol");

			return FALSE;
		}
	}

	if (m_nMaxReportSize == 0)
	{
		m_nMaxReportSize = nMaxReportSize;
		assert (m_nMaxReportSize > 0);

		assert (m_pReportBuffer == 0);
		m_pReportBuffer = new u8[m_nMaxReportSize];
	}
	assert (m_pReportBuffer != 0);

	return TRUE;
}

boolean CUSBHIDDevice::SendToEndpointOut (const void *pBuffer, unsigned nBufSize, unsigned nTimeoutMs)
{
	if (m_pEndpointOut == 0)
	{
		return FALSE;
	}

	assert (pBuffer != 0);
	assert (nBufSize > 0);
	if (GetHost ()->Transfer (m_pEndpointOut, (void *) pBuffer, nBufSize, nTimeoutMs) < 0)
	{
		return FALSE;
	}

	return TRUE;
}

boolean CUSBHIDDevice::SendToEndpointOutAsync (const void *pBuffer, unsigned nBufSize,
					       unsigned nTimeoutMs)
{
	if (m_pEndpointOut == 0)
	{
		return FALSE;
	}

	assert (pBuffer != 0);
	assert (nBufSize > 0);
	u8 *pTempBuffer = new u8[nBufSize];
	assert (pTempBuffer != 0);
	memcpy (pTempBuffer, pBuffer, nBufSize);

	CUSBRequest *pURB = new CUSBRequest (m_pEndpointOut, pTempBuffer, nBufSize);
	assert (pURB != 0);
	pURB->SetCompletionRoutine (SendCompletionRoutine, pTempBuffer, this);

	return GetHost ()->SubmitAsyncRequest (pURB, nTimeoutMs);
}

void CUSBHIDDevice::SendCompletionRoutine (CUSBRequest *pURB, void *pParam, void *pContext)
{
	assert (pURB != 0);
	if (!pURB->GetStatus ())
	{
		CLogger::Get ()->Write (FromUSBHID, LogWarning, "SendToEndpointOut failed");
	}

	delete pURB;

	u8 *pTempBuffer = (u8 *) pParam;
	assert (pTempBuffer != 0);
	delete [] pTempBuffer;
}

int CUSBHIDDevice::ReceiveFromEndpointIn (void *pBuffer, unsigned nBufSize, unsigned nTimeoutMs)
{
	assert (m_pReportEndpoint != 0);
	assert (pBuffer != 0);
	assert (nBufSize > 0);
	return GetHost ()->Transfer (m_pReportEndpoint, pBuffer, nBufSize, nTimeoutMs);
}

boolean CUSBHIDDevice::StartRequest (void)
{
	assert (m_pReportEndpoint != 0);
	assert (m_pReportBuffer != 0);
	
	assert (m_nMaxReportSize > 0);
	CUSBRequest *pURB = new CUSBRequest (m_pReportEndpoint, m_pReportBuffer, m_nMaxReportSize);
	assert (pURB != 0);
	pURB->SetCompletionRoutine (CompletionStub, 0, this);
	
	return GetHost ()->SubmitAsyncRequest (pURB);
}

void CUSBHIDDevice::CompletionRoutine (CUSBRequest *pURB)
{
	assert (pURB != 0);

	boolean bRestart = TRUE;

	if (pURB->GetStatus () != 0)
	{
		ReportHandler (m_pReportBuffer, pURB->GetResultLength ());
	}
	else
	{
		if (!CUSBHostController::IsPlugAndPlay ())
		{
			ReportHandler (0, 0);
		}
		else
		{
			if (pURB->GetUSBError () != USBErrorFrameOverrun)
			{
				bRestart = FALSE;
			}
		}
	}

	delete pURB;

	if (   bRestart
	    && !StartRequest ())
	{
		CLogger::Get ()->Write (FromUSBHID, LogError, "Cannot restart request");
	}
}

void CUSBHIDDevice::CompletionStub (CUSBRequest *pURB, void *pParam, void *pContext)
{
	CUSBHIDDevice *pThis = (CUSBHIDDevice *) pContext;
	assert (pThis != 0);
	
	pThis->CompletionRoutine (pURB);
}
