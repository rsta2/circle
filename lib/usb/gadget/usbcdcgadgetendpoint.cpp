//
// usbcdcgadgetendpoint.cpp
//
// This file by Sebastien Nicolas <seba1978@gmx.de>
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2023-2024  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/gadget/usbcdcgadgetendpoint.h>
#include <circle/usb/gadget/usbcdcgadget.h>
#include <circle/usb/usbserial.h>
#include <assert.h>

CUSBCDCGadgetEndpoint::CUSBCDCGadgetEndpoint (const TUSBEndpointDescriptor *pDesc,
						CUSBCDCGadget *pGadget)
:	CDWUSBGadgetEndpoint (pDesc, pGadget),
	m_pInterface (nullptr),
	m_nStatus (0),
	m_bInActive (FALSE),
	m_pQueue (nullptr),
	m_nInPtr (0),
	m_nOutPtr (0)
{
	m_pQueue = new u8[QueueSize];
	assert (m_pQueue);
}

CUSBCDCGadgetEndpoint::~CUSBCDCGadgetEndpoint (void)
{
	delete [] m_pQueue;
	m_pQueue = nullptr;
}

void CUSBCDCGadgetEndpoint::AttachInterface (CUSBSerialDevice *pInterface)
{
	m_nStatus = 0;

	assert (!m_pInterface);
	m_pInterface = pInterface;
	assert (m_pInterface);

	if (GetDirection () == DirectionIn)
	{
		m_pInterface->RegisterWriteHandler (WriteHandler, this);
	}
	else
	{
		m_pInterface->RegisterReadHandler (ReadHandler, this);
	}
}

void CUSBCDCGadgetEndpoint::OnActivate (void)
{
	if (GetDirection () == DirectionOut)
	{
		BeginTransfer (TransferDataOut, m_OutBuffer, MaxOutMessageSize);
	}
}

void CUSBCDCGadgetEndpoint::OnTransferComplete (boolean bIn, size_t nLength)
{
	if (!bIn)
	{
		m_SpinLock.Acquire ();

		unsigned nBytesFree = GetQueueBytesFree ();
		if (nLength > nBytesFree)
		{
			nLength = nBytesFree;

			m_nStatus = -1;		// RX overrun
		}

		Enqueue (m_OutBuffer, nLength);

		m_SpinLock.Release ();

		BeginTransfer (TransferDataOut, m_OutBuffer, MaxOutMessageSize);
	}
	else
	{
		m_SpinLock.Acquire ();

		if (m_bInActive)
		{
			unsigned nBytesAvail = GetQueueBytesAvail ();
			if (nBytesAvail)
			{
				if (nBytesAvail > MaxInMessageSize)
				{
					nBytesAvail = MaxInMessageSize;
				}

				Dequeue (m_InBuffer, nBytesAvail);

				BeginTransfer (TransferDataIn, m_InBuffer, nBytesAvail);
			}
			else
			{
				m_bInActive = FALSE;
			}
		}

		m_SpinLock.Release ();
	}
}

void CUSBCDCGadgetEndpoint::OnSuspend (void)
{
	if (GetDirection () == DirectionIn)
	{
		m_SpinLock.Acquire ();

		m_bInActive = FALSE;
		m_nInPtr = 0;
		m_nOutPtr = 0;

		m_SpinLock.Release ();
	}
}

int CUSBCDCGadgetEndpoint::Write (const void *pData, unsigned nLength)
{
	assert (pData != 0);
	assert (nLength > 0);

	m_SpinLock.Acquire ();

	if (m_nStatus)
	{
		int nStatus = m_nStatus;
		m_nStatus = 0;

		m_SpinLock.Release ();

		return nStatus;
	}

	unsigned nBytesFree = GetQueueBytesFree ();
	if (!nBytesFree)
	{
		m_SpinLock.Release ();

		return 0;
	}

	if (nLength > nBytesFree)
	{
		nLength = nBytesFree;
	}

	Enqueue (pData, nLength);

	if (m_bInActive)
	{
		m_SpinLock.Release ();

		return nLength;
	}

	m_bInActive = TRUE;

	unsigned nBytesAvail = GetQueueBytesAvail ();
	if (nBytesAvail > MaxInMessageSize)
	{
		nBytesAvail = MaxInMessageSize;
	}

	Dequeue (m_InBuffer, nBytesAvail);

	m_SpinLock.Release ();

	BeginTransfer (TransferDataIn, m_InBuffer, nBytesAvail);

	return nLength;
}

int CUSBCDCGadgetEndpoint::Read (void *pBuffer, unsigned nLength)
{
	m_SpinLock.Acquire ();

	if (m_nStatus)
	{
		int nStatus = m_nStatus;
		m_nStatus = 0;

		m_SpinLock.Release ();

		return nStatus;
	}

	unsigned nBytesAvail = GetQueueBytesAvail ();
	if (!nBytesAvail)
	{
		m_SpinLock.Release ();

		return 0;
	}

	if (nBytesAvail > nLength)
	{
		nBytesAvail = nLength;
	}

	Dequeue (pBuffer, nBytesAvail);

	m_SpinLock.Release ();

	return nBytesAvail;
}

int CUSBCDCGadgetEndpoint::WriteHandler (const void *pBuffer, size_t nCount, void *pParam)
{
	CUSBCDCGadgetEndpoint *pThis = static_cast<CUSBCDCGadgetEndpoint *> (pParam);
	assert (pThis);

	return pThis->Write (pBuffer, nCount);
}

int CUSBCDCGadgetEndpoint::ReadHandler (void *pBuffer, size_t nCount, void *pParam)
{
	CUSBCDCGadgetEndpoint *pThis = static_cast<CUSBCDCGadgetEndpoint *> (pParam);
	assert (pThis);

	return pThis->Read (pBuffer, nCount);
}

unsigned CUSBCDCGadgetEndpoint::GetQueueBytesFree (void)
{
	assert (m_nInPtr < QueueSize);
	assert (m_nOutPtr < QueueSize);

	if (m_nOutPtr <= m_nInPtr)
	{
		return QueueSize+m_nOutPtr-m_nInPtr-1;
	}

	return m_nOutPtr-m_nInPtr-1;
}

unsigned CUSBCDCGadgetEndpoint::GetQueueBytesAvail (void)
{
	assert (m_nInPtr < QueueSize);
	assert (m_nOutPtr < QueueSize);

	if (m_nInPtr < m_nOutPtr)
	{
		return QueueSize+m_nInPtr-m_nOutPtr;
	}

	return m_nInPtr-m_nOutPtr;
}

void CUSBCDCGadgetEndpoint::Enqueue (const void *pBuffer, unsigned nCount)
{
	const u8 *p = static_cast<const u8 *> (pBuffer);
	assert (p != 0);
	assert (m_pQueue != 0);

	assert (nCount > 0);
	while (nCount-- > 0)
	{
		m_pQueue[m_nInPtr] = *p++;

		if (++m_nInPtr == QueueSize)
		{
			m_nInPtr = 0;
		}
	}
}

void CUSBCDCGadgetEndpoint::Dequeue (void *pBuffer, unsigned nCount)
{
	u8 *p = static_cast<u8 *> (pBuffer);
	assert (p != 0);
	assert (m_pQueue != 0);

	assert (nCount > 0);
	while (nCount-- > 0)
	{
		*p++ = m_pQueue[m_nOutPtr];

		if (++m_nOutPtr == QueueSize)
		{
			m_nOutPtr = 0;
		}
	}
}
