//
// usbhiddevice.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2016  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/usbhostcontroller.h>
#include <circle/logger.h>
#include <assert.h>

static const char FromUSBHID[] = "usbhid";

CUSBHIDDevice::CUSBHIDDevice (CUSBFunction *pFunction, unsigned nReportSize)
:	CUSBFunction (pFunction),
	m_nReportSize (nReportSize),
	m_pReportEndpoint (0),
	m_pURB (0),
	m_pReportBuffer (0)
{
	if (m_nReportSize > 0)
	{
		m_pReportBuffer = new u8[m_nReportSize];
		assert (m_pReportBuffer != 0);
	}
}

CUSBHIDDevice::~CUSBHIDDevice (void)
{
	delete [] m_pReportBuffer;
	m_pReportBuffer = 0;

	delete m_pReportEndpoint;
	m_pReportEndpoint = 0;
}

boolean CUSBHIDDevice::Configure (unsigned nReportSize)
{
	if (GetNumEndpoints () < 1)
	{
		ConfigurationError (FromUSBHID);

		return FALSE;
	}

	const TUSBEndpointDescriptor *pEndpointDesc;
	while ((pEndpointDesc = (TUSBEndpointDescriptor *) GetDescriptor (DESCRIPTOR_ENDPOINT)) != 0)
	{
		if (   (pEndpointDesc->bEndpointAddress & 0x80) == 0x80		// Input EP
		    && (pEndpointDesc->bmAttributes     & 0x3F)	== 0x03)	// Interrupt EP
		{
			break;
		}
	}

	if (pEndpointDesc == 0)
	{
		ConfigurationError (FromUSBHID);

		return FALSE;
	}

	assert (m_pReportEndpoint == 0);
	m_pReportEndpoint = new CUSBEndpoint (GetDevice (), pEndpointDesc);
	assert (m_pReportEndpoint != 0);

	if (!CUSBFunction::Configure ())
	{
		CLogger::Get ()->Write (FromUSBHID, LogError, "Cannot set interface");

		return FALSE;
	}

	if (GetInterfaceSubClass () == 1)	// boot class
	{
		if (GetHost ()->ControlMessage (GetEndpoint0 (),
						REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_INTERFACE,
						SET_PROTOCOL, BOOT_PROTOCOL,
						GetInterfaceNumber (), 0, 0) < 0)
		{
			CLogger::Get ()->Write (FromUSBHID, LogError, "Cannot set boot protocol");

			return FALSE;
		}
	}

	if (m_nReportSize == 0)
	{
		m_nReportSize = nReportSize;
		assert (m_nReportSize > 0);

		assert (m_pReportBuffer == 0);
		m_pReportBuffer = new u8[m_nReportSize];
	}
	assert (m_pReportBuffer != 0);

	return StartRequest ();
}

boolean CUSBHIDDevice::StartRequest (void)
{
	assert (m_pReportEndpoint != 0);
	assert (m_pReportBuffer != 0);
	
	assert (m_pURB == 0);
	assert (m_nReportSize > 0);
	m_pURB = new CUSBRequest (m_pReportEndpoint, m_pReportBuffer, m_nReportSize);
	assert (m_pURB != 0);
	m_pURB->SetCompletionRoutine (CompletionStub, 0, this);
	
	return GetHost ()->SubmitAsyncRequest (m_pURB);
}

void CUSBHIDDevice::CompletionRoutine (CUSBRequest *pURB)
{
	assert (pURB != 0);
	assert (m_pURB == pURB);

	if (   pURB->GetStatus () != 0
	    && pURB->GetResultLength () == m_nReportSize)
	{
		ReportHandler (m_pReportBuffer);
	}
	else
	{
		ReportHandler (0);
	}

	delete m_pURB;
	m_pURB = 0;
	
	if (!StartRequest ())
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
