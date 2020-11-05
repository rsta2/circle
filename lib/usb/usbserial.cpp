//
// usbserial.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2020  H. Kocevar <hinxx@protonmail.com>
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
#include <circle/usb/usbserial.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/devicenameservice.h>
#include <circle/logger.h>
#include <assert.h>

CNumberPool CUSBSerialDevice::s_DeviceNumberPool (1);

static const char FromSerial[] = "userial";
static const char DevicePrefix[] = "utty";

CUSBSerialDevice::CUSBSerialDevice (CUSBFunction *pFunction)
:	CUSBFunction (pFunction),
	m_nBaudRate (9600),
	m_nDataBits (USBSerialDataBits8),
	m_nParity (USBSerialParityNone),
	m_nStopBits (USBSerialStopBits1),
	m_pEndpointIn (0),
	m_pEndpointOut (0),
	m_nDeviceNumber (0)
{
}

CUSBSerialDevice::~CUSBSerialDevice (void)
{
	if (m_nDeviceNumber != 0)
	{
		CDeviceNameService::Get ()->RemoveDevice (DevicePrefix, m_nDeviceNumber, FALSE);

		s_DeviceNumberPool.FreeNumber (m_nDeviceNumber);
	}

	delete m_pEndpointOut;
	m_pEndpointOut =  0;
	
	delete m_pEndpointIn;
	m_pEndpointIn = 0;
}

boolean CUSBSerialDevice::Configure (void)
{
	const TUSBEndpointDescriptor *pEndpointDesc;
	while ((pEndpointDesc = (TUSBEndpointDescriptor *) GetDescriptor (DESCRIPTOR_ENDPOINT)) != 0)
	{
		if ((pEndpointDesc->bmAttributes & 0x3F) == 0x02)		// Bulk
		{
			if ((pEndpointDesc->bEndpointAddress & 0x80) == 0x80)	// Input
			{
				if (m_pEndpointIn != 0)
				{
					ConfigurationError (FromSerial);

					return FALSE;
				}

				m_pEndpointIn = new CUSBEndpoint (GetDevice (), pEndpointDesc);
			}
			else							// Output
			{
				if (m_pEndpointOut != 0)
				{
					ConfigurationError (FromSerial);

					return FALSE;
				}

				m_pEndpointOut = new CUSBEndpoint (GetDevice (), pEndpointDesc);
			}
		}
	}

	if (   m_pEndpointIn == 0
	    || m_pEndpointOut == 0)
	{
		ConfigurationError (FromSerial);

		return FALSE;
	}

	if (!CUSBFunction::Configure ())
	{
		CLogger::Get ()->Write (FromSerial, LogError, "Cannot set interface");

		return FALSE;
	}

	assert (m_nDeviceNumber == 0);
	m_nDeviceNumber = s_DeviceNumberPool.AllocateNumber (TRUE, FromSerial);

	CDeviceNameService::Get ()->AddDevice (DevicePrefix, m_nDeviceNumber, this, FALSE);

	return TRUE;
}

int CUSBSerialDevice::Write (const void *pBuffer, size_t nCount)
{
	assert (pBuffer != 0);
	assert (nCount > 0);
	
	CUSBHostController *pHost = GetHost ();
	assert (pHost != 0);

	if (pHost->Transfer (m_pEndpointOut, (void *) pBuffer, nCount) < 0)
	{
		return -1;
	}

	return nCount;
}

int CUSBSerialDevice::Read (void *pBuffer, size_t nCount)
{
	assert (pBuffer != 0);
	assert (nCount > 0);

	CUSBHostController *pHost = GetHost ();
	assert (pHost != 0);

	if (pHost->Transfer (m_pEndpointIn, (void *) pBuffer, nCount) < 0)
	{
		return -1;
	}

	return nCount;
}


boolean CUSBSerialDevice::SetBaudRate (unsigned nBaudRate)
{
	(void)nBaudRate;
	return TRUE;
}

boolean CUSBSerialDevice::SetLineProperties (TUSBSerialDataBits nDataBits, TUSBSerialParity nParity, TUSBSerialStopBits nStopBits)
{
	(void)nDataBits;
	(void)nParity;
	(void)nStopBits;
	return TRUE;
}
