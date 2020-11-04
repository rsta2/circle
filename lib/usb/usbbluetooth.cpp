//
// usbbluetooth.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2020  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/usbbluetooth.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/devicenameservice.h>
#include <circle/logger.h>
#include <circle/string.h>
#include <assert.h>

static const char FromBluetooth[] = "btusb";
static const char DevicePrefix[] = "ubt";

CNumberPool CUSBBluetoothDevice::s_DeviceNumberPool (1);

CUSBBluetoothDevice::CUSBBluetoothDevice (CUSBFunction *pFunction)
:	CUSBFunction (pFunction),
	m_pEndpointInterrupt (0),
	m_pEndpointBulkIn (0),
	m_pEndpointBulkOut (0),
	m_pEventBuffer (0),
	m_pEventHandler (0),
	m_nDeviceNumber (0)
{
}

CUSBBluetoothDevice::~CUSBBluetoothDevice (void)
{
	m_pEventHandler = 0;

	if (m_nDeviceNumber != 0)
	{
		CDeviceNameService::Get ()->RemoveDevice (DevicePrefix, m_nDeviceNumber, FALSE);

		s_DeviceNumberPool.FreeNumber (m_nDeviceNumber);
	}

	delete [] m_pEventBuffer;
	m_pEventBuffer = 0;
	
	delete m_pEndpointBulkOut;
	m_pEndpointBulkOut = 0;

	delete m_pEndpointBulkIn;
	m_pEndpointBulkIn = 0;

	delete m_pEndpointInterrupt;
	m_pEndpointInterrupt = 0;
}

boolean CUSBBluetoothDevice::Configure (void)
{
	if (GetInterfaceNumber () != 0)
	{
		CLogger::Get ()->Write (FromBluetooth, LogWarning, "Voice channels are not supported");

		return FALSE;
	}

	if (GetNumEndpoints () != 3)
	{
		ConfigurationError (FromBluetooth);

		return FALSE;
	}

	const TUSBEndpointDescriptor *pEndpointDesc;
	while ((pEndpointDesc = (TUSBEndpointDescriptor *) GetDescriptor (DESCRIPTOR_ENDPOINT)) != 0)
	{
		if ((pEndpointDesc->bmAttributes & 0x3F) == 0x02)		// Bulk
		{
			if ((pEndpointDesc->bEndpointAddress & 0x80) == 0x80)	// Input
			{
				if (m_pEndpointBulkIn != 0)
				{
					ConfigurationError (FromBluetooth);

					return FALSE;
				}

				m_pEndpointBulkIn = new CUSBEndpoint (GetDevice (), pEndpointDesc);
			}
			else							// Output
			{
				if (m_pEndpointBulkOut != 0)
				{
					ConfigurationError (FromBluetooth);

					return FALSE;
				}

				m_pEndpointBulkOut = new CUSBEndpoint (GetDevice (), pEndpointDesc);
			}
		}
		else if ((pEndpointDesc->bmAttributes & 0x3F) == 0x03)		// Interrupt
		{
			if (m_pEndpointInterrupt != 0)
			{
				ConfigurationError (FromBluetooth);

				return FALSE;
			}

			m_pEndpointInterrupt = new CUSBEndpoint (GetDevice (), pEndpointDesc);
		}
	}

	if (   m_pEndpointBulkIn    == 0
	    || m_pEndpointBulkOut   == 0
	    || m_pEndpointInterrupt == 0)
	{
		ConfigurationError (FromBluetooth);

		return FALSE;
	}

	if (!CUSBFunction::Configure ())
	{
		CLogger::Get ()->Write (FromBluetooth, LogError, "Cannot set interface");

		return FALSE;
	}

	m_pEventBuffer = new u8[m_pEndpointInterrupt->GetMaxPacketSize ()];
	assert (m_pEventBuffer != 0);

	assert (m_nDeviceNumber == 0);
	m_nDeviceNumber = s_DeviceNumberPool.AllocateNumber (TRUE, FromBluetooth);

	CDeviceNameService::Get ()->AddDevice (DevicePrefix, m_nDeviceNumber, this, FALSE);

	return TRUE;
}

boolean CUSBBluetoothDevice::SendHCICommand (const void *pBuffer, unsigned nLength)
{
	if (GetHost ()->ControlMessage (GetEndpoint0 (), REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_DEVICE,
					0, 0, 0, (void *) pBuffer, nLength) < 0)
	{
		return FALSE;
	}

	return TRUE;
}

void CUSBBluetoothDevice::RegisterHCIEventHandler (TBTHCIEventHandler *pHandler)
{
	m_pEventHandler = pHandler;
	assert (m_pEventHandler != 0);

	StartRequest ();
}

boolean CUSBBluetoothDevice::StartRequest (void)
{
	assert (m_pEndpointInterrupt != 0);
	assert (m_pEventBuffer != 0);

	CUSBRequest *pURB = new CUSBRequest (m_pEndpointInterrupt, m_pEventBuffer,
					     m_pEndpointInterrupt->GetMaxPacketSize ());
	assert (pURB != 0);

	pURB->SetCompletionRoutine (CompletionStub, 0, this);
	
	return GetHost ()->SubmitAsyncRequest (pURB);
}

void CUSBBluetoothDevice::CompletionRoutine (CUSBRequest *pURB)
{
	assert (pURB != 0);

	boolean bRestart = TRUE;

	if (pURB->GetStatus () != 0)
	{
		assert (m_pEventHandler != 0);
		assert (m_pEventBuffer != 0);
		(*m_pEventHandler) (m_pEventBuffer, pURB->GetResultLength ());
	}
	else
	{
		CLogger::Get ()->Write (FromBluetooth, LogWarning, "Request failed");

		bRestart = FALSE;
	}

	delete pURB;

	if (   bRestart
	    && !StartRequest ())
	{
		CLogger::Get ()->Write (FromBluetooth, LogError, "Cannot restart request");
	}
}

void CUSBBluetoothDevice::CompletionStub (CUSBRequest *pURB, void *pParam, void *pContext)
{
	CUSBBluetoothDevice *pThis = (CUSBBluetoothDevice *) pContext;
	assert (pThis != 0);
	
	pThis->CompletionRoutine (pURB);
}
