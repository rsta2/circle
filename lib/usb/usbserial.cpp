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
#include <circle/devicenameservice.h>
#include <assert.h>

CNumberPool CUSBSerialDevice::s_DeviceNumberPool (1);

static const char From[] = "userial";
static const char DevicePrefix[] = "utty";

CUSBSerialDevice::CUSBSerialDevice (void)
:	m_pWriteHandler (nullptr),
	m_pReadHandler (nullptr),
	m_pSetBaudRateHandler (nullptr),
	m_pSetLinePropertiesHandler (nullptr),
	m_nOptions (0),
	m_nDeviceNumber (s_DeviceNumberPool.AllocateNumber (TRUE, From))
{
	CDeviceNameService::Get ()->AddDevice (DevicePrefix, m_nDeviceNumber, this, FALSE);
}

CUSBSerialDevice::~CUSBSerialDevice (void)
{
	CDeviceNameService::Get ()->RemoveDevice (DevicePrefix, m_nDeviceNumber, FALSE);
	s_DeviceNumberPool.FreeNumber (m_nDeviceNumber);

	m_pWriteHandler = nullptr;
	m_pReadHandler = nullptr;
	m_pSetBaudRateHandler = nullptr;
	m_pSetLinePropertiesHandler = nullptr;
}

int CUSBSerialDevice::Write (const void *pBuffer, size_t nCount)
{
	assert (m_pWriteHandler);

	if (!(m_nOptions & SERIAL_OPTION_ONLCR))
	{
		return (*m_pWriteHandler) (pBuffer, nCount, m_pWriteParam);
	}

	const char *pIn = reinterpret_cast<const char *> (pBuffer);
	assert (pIn);

	char Buffer[nCount*2];
	char *pOut = Buffer;

	while (nCount--)
	{
		if (*pIn == '\n')
		{
			*pOut++ = '\r';
		}

		*pOut++ = *pIn++;
	}

	return (*m_pWriteHandler) (Buffer, pOut-Buffer, m_pWriteParam);
}

int CUSBSerialDevice::Read (void *pBuffer, size_t nCount)
{
	assert (m_pReadHandler);
	return (*m_pReadHandler) (pBuffer, nCount, m_pReadParam);
}

boolean CUSBSerialDevice::SetBaudRate (unsigned nBaudRate)
{
	if (!m_pSetBaudRateHandler)
	{
		return TRUE;
	}

	return (*m_pSetBaudRateHandler) (nBaudRate, m_pSetBaudRateParam);
}

boolean CUSBSerialDevice::SetLineProperties (TUSBSerialDataBits nDataBits,
					     TUSBSerialParity nParity,
					     TUSBSerialStopBits nStopBits)
{
	if (!m_pSetLinePropertiesHandler)
	{
		return TRUE;
	}

	return (*m_pSetLinePropertiesHandler) (nDataBits, nParity, nStopBits,
					       m_pSetLinePropertiesParam);
}

unsigned CUSBSerialDevice::GetOptions (void) const
{
	return m_nOptions;
}

void CUSBSerialDevice::SetOptions (unsigned nOptions)
{
	m_nOptions = nOptions;
}

void CUSBSerialDevice::RegisterWriteHandler (TWriteHandler *pHandler, void *pParam)
{
	m_pWriteParam = pParam;

	assert (!m_pWriteHandler);
	m_pWriteHandler = pHandler;
	assert (m_pWriteHandler);
}

void CUSBSerialDevice::RegisterReadHandler (TReadHandler *pHandler, void *pParam)
{
	m_pReadParam = pParam;

	assert (!m_pReadHandler);
	m_pReadHandler = pHandler;
	assert (m_pReadHandler);
}

void CUSBSerialDevice::RegisterSetBaudRateHandler (TSetBaudRateHandler *pHandler, void *pParam)
{
	m_pSetBaudRateParam = pParam;

	assert (!m_pSetBaudRateHandler);
	m_pSetBaudRateHandler = pHandler;
	assert (m_pSetBaudRateHandler);
}

void CUSBSerialDevice::RegisterSetLinePropertiesHandler (TSetLinePropertiesHandler *pHandler,
							 void *pParam)
{
	m_pSetLinePropertiesParam = pParam;

	assert (!m_pSetLinePropertiesHandler);
	m_pSetLinePropertiesHandler = pHandler;
	assert (m_pSetLinePropertiesHandler);
}
