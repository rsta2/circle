//
// usbserialhost.cpp
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
#include <circle/synchronize.h>
#include <circle/util.h>
#include <circle/usb/usbserialhost.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/usb/usbrequest.h>
#include <circle/logger.h>
#include <assert.h>

LOGMODULE ("userialhost");

CUSBSerialHostDevice::CUSBSerialHostDevice (CUSBFunction *pFunction, size_t nReadHeaderBytes)
:	CUSBFunction (pFunction),
	m_nBaudRate (9600),
	m_nDataBits (USBSerialDataBits8),
	m_nParity (USBSerialParityNone),
	m_nStopBits (USBSerialStopBits1),
	m_pInterface (0),
	m_nReadHeaderBytes (nReadHeaderBytes),
	m_pEndpointIn (0),
	m_pEndpointOut (0),
	m_pBufferIn (0),
	m_nBufferInSize (0),
	m_nBufferInValid (0),
	m_nBufferInPtr (0),
	m_bInRequestActive (FALSE)
{
}

CUSBSerialHostDevice::~CUSBSerialHostDevice (void)
{
	delete m_pInterface;
	m_pInterface = 0;

	delete m_pEndpointOut;
	m_pEndpointOut =  0;
	
	delete m_pEndpointIn;
	m_pEndpointIn = 0;

	delete [] m_pBufferIn;
	m_pBufferIn = 0;
	m_nBufferInSize = 0;
}

boolean CUSBSerialHostDevice::Configure (void)
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
					ConfigurationError (From);

					return FALSE;
				}

				m_pEndpointIn = new CUSBEndpoint (GetDevice (), pEndpointDesc);
			}
			else							// Output
			{
				if (m_pEndpointOut != 0)
				{
					ConfigurationError (From);

					return FALSE;
				}

				m_pEndpointOut = new CUSBEndpoint (GetDevice (), pEndpointDesc);
			}
		}
	}

	if (   m_pEndpointIn == 0
	    || m_pEndpointOut == 0)
	{
		ConfigurationError (From);

		return FALSE;
	}

	m_nBufferInSize = m_pEndpointIn->GetMaxPacketSize ();
	m_pBufferIn = new u8[m_nBufferInSize];
	assert (m_pBufferIn != 0);

	if (!CUSBFunction::Configure ())
	{
		LOGERR ("Cannot set interface");

		return FALSE;
	}

	// attach interface device
	assert (m_pInterface == 0);
	m_pInterface = new CUSBSerialDevice;
	assert (m_pInterface != 0);

	m_pInterface->RegisterWriteHandler (WriteHandler, this);
	m_pInterface->RegisterReadHandler (ReadHandler, this);
	m_pInterface->RegisterSetBaudRateHandler (SetBaudRateHandler, this);
	m_pInterface->RegisterSetLinePropertiesHandler (SetLinePropertiesHandler, this);

	return TRUE;
}

int CUSBSerialHostDevice::Write (const void *pBuffer, size_t nCount)
{
	assert (pBuffer != 0);
	assert (nCount > 0);

#if RASPPI <= 3
	// USB host controller does not allow concurrent split transactions
	// to same device. Thus wait for completion of pending IN request.
	do
	{
		DataMemBarrier ();
	}
	while (m_bInRequestActive);
#endif

	CUSBHostController *pHost = GetHost ();
	assert (pHost != 0);

	DMA_BUFFER (u8, BufferOut, nCount);
	memcpy (BufferOut, pBuffer, nCount);

	assert (m_pEndpointOut != 0);
	int nActual = pHost->Transfer (m_pEndpointOut, BufferOut, nCount);
	if (nActual < 0)
	{
		LOGWARN ("USB write failed");
	}

	return nActual;
}

int CUSBSerialHostDevice::Read (void *pBuffer, size_t nCount)
{
	assert (pBuffer != 0);
	assert (nCount > 0);

	DataMemBarrier ();

	if (m_bInRequestActive)
	{
		return 0;
	}

	assert (m_pBufferIn != 0);
	assert (m_nBufferInSize > 0);
	assert (m_nBufferInValid <= m_nBufferInSize);
	assert (m_nBufferInPtr <= m_nBufferInValid);

	if (m_nBufferInPtr == m_nBufferInValid)		// buffer empty?
	{
		CUSBHostController *pHost = GetHost ();
		assert (pHost != 0);

		assert (m_pEndpointIn != 0);
		CUSBRequest *pURB = new CUSBRequest (m_pEndpointIn, m_pBufferIn, m_nBufferInSize);
		assert (pURB != 0);

		// do not retry if request cannot be served immediately
		pURB->SetCompleteOnNAK ();

		pURB->SetCompletionRoutine (CompletionStub, 0, this);
		m_bInRequestActive = TRUE;

		if (!pHost->SubmitAsyncRequest (pURB))
		{
			LOGWARN ("USB read failed");

			m_bInRequestActive = FALSE;

			delete pURB;

			return -1;
		}

		return 0;
	}

	size_t nRemain = m_nBufferInValid - m_nBufferInPtr;
	if (nRemain < nCount)
	{
		nCount = nRemain;
	}

	assert (nCount > 0);
	memcpy (pBuffer, m_pBufferIn + m_nBufferInPtr, nCount);
	m_nBufferInPtr += nCount;

	return nCount;
}

boolean CUSBSerialHostDevice::SetBaudRate (unsigned nBaudRate)
{
	return TRUE;
}

boolean CUSBSerialHostDevice::SetLineProperties (TUSBSerialDataBits nDataBits,
						 TUSBSerialParity nParity,
						 TUSBSerialStopBits nStopBits)
{
	return TRUE;
}

void CUSBSerialHostDevice::CompletionRoutine (CUSBRequest *pURB)
{
	assert (pURB != 0);
	assert (m_bInRequestActive);
	assert (m_nBufferInValid == m_nBufferInPtr);

	if (pURB->GetStatus () != 0)
	{
		m_nBufferInValid = pURB->GetResultLength ();
		m_nBufferInPtr = m_nReadHeaderBytes;

		if (   m_nBufferInValid == 0
		    || m_nBufferInValid == m_nBufferInPtr)
		{
			m_nBufferInValid = 0;
			m_nBufferInPtr = 0;
		}
		else if (m_nBufferInValid < m_nBufferInPtr)
		{
			LOGWARN ("Missing read header");

			m_nBufferInValid = 0;
			m_nBufferInPtr = 0;
		}
	}

	delete pURB;

	m_bInRequestActive = FALSE;
	DataSyncBarrier ();
}

void CUSBSerialHostDevice::CompletionStub (CUSBRequest *pURB, void *pParam, void *pContext)
{
	CUSBSerialHostDevice *pThis = static_cast <CUSBSerialHostDevice *> (pContext);
	assert (pThis != 0);

	pThis->CompletionRoutine (pURB);
}

int CUSBSerialHostDevice::WriteHandler (const void *pBuffer, size_t nCount, void *pParam)
{
	CUSBSerialHostDevice *pThis = static_cast <CUSBSerialHostDevice *> (pParam);
	assert (pThis != 0);

	return pThis->Write (pBuffer, nCount);
}

int CUSBSerialHostDevice::ReadHandler (void *pBuffer, size_t nCount, void *pParam)
{
	CUSBSerialHostDevice *pThis = static_cast <CUSBSerialHostDevice *> (pParam);
	assert (pThis != 0);

	return pThis->Read (pBuffer, nCount);
}

boolean CUSBSerialHostDevice::SetBaudRateHandler (unsigned nBaudRate, void *pParam)
{
	CUSBSerialHostDevice *pThis = static_cast <CUSBSerialHostDevice *> (pParam);
	assert (pThis != 0);

	return pThis->SetBaudRate (nBaudRate);
}

boolean CUSBSerialHostDevice::SetLinePropertiesHandler (TUSBSerialDataBits nDataBits,
							TUSBSerialParity nParity,
							TUSBSerialStopBits nStopBits,
							void *pParam)
{
	CUSBSerialHostDevice *pThis = static_cast <CUSBSerialHostDevice *> (pParam);
	assert (pThis != 0);

	return pThis->SetLineProperties (nDataBits, nParity, nStopBits);
}
