//
// netbuffer.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2025  R. Stange <rsta2@gmx.net>
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
#include <circle/net/netbuffer.h>
#include <circle/logger.h>
#include <circle/string.h>
#include <circle/debug.h>
#include <circle/util.h>
#include <assert.h>

// Various header lengths
#define ETHERNET_HEADER_LEN	(6+6+2)
#define IP_HEADER_LEN		(5*4)		// no option
#define IP_RA_HEADER_LEN	(5*4+4)		// with Router Alert option
#define TCP_HEADER_LEN		(5*4)		// no option
#define TCP_MSS_HEADER_LEN	(5*4+4)		// with MSS option
#define UDP_HEADER_LEN		(2*4)		// no option

LOGMODULE ("netbuf");

CNetBuffer::CNetBuffer (TPurpose Purpose, size_t ulLength, const void *pBuffer)
:	m_pNext (nullptr),
	m_Purpose (Purpose),
	m_bValid (TRUE),
	m_pHead (m_Buffer + HeaderReserve),
	m_ulLength (ulLength),
	m_ulPrivateDataLength (0)
{
	switch (m_Purpose)	// This is done for cache alignment.
	{
	case Receive:
	case ARPSend:
	case LLRawSend:
		break;

	case TCPSend:
		m_pHead += ETHERNET_HEADER_LEN + IP_HEADER_LEN + TCP_HEADER_LEN;
		break;

	case TCPSendMSS:
		m_pHead += ETHERNET_HEADER_LEN + IP_HEADER_LEN + TCP_MSS_HEADER_LEN;
		break;

	case UDPSend:
		m_pHead += ETHERNET_HEADER_LEN + IP_HEADER_LEN + UDP_HEADER_LEN;
		break;

	case IGMPSend:
		m_pHead += ETHERNET_HEADER_LEN + IP_RA_HEADER_LEN;
		break;
	}

	assert (m_pHead + ulLength <= m_Buffer + BufferSize);

	if (pBuffer)
	{
		assert (ulLength);
		memcpy (m_pHead, pBuffer, ulLength);
	}
}

CNetBuffer::CNetBuffer (CNetBuffer &rNetBuffer)
:	m_pNext (nullptr),
	m_Purpose (rNetBuffer.m_Purpose),
	m_bValid (rNetBuffer.m_bValid),
	m_pHead (m_Buffer + (rNetBuffer.m_pHead - rNetBuffer.m_Buffer)),
	m_ulLength (rNetBuffer.m_ulLength),
	m_ulPrivateDataLength (rNetBuffer.m_ulPrivateDataLength)
{
	assert (m_bValid);
	assert (!rNetBuffer.m_pNext);	// not enqueued

	assert (m_pHead >= m_Buffer);
	assert (m_pHead + m_ulLength <= m_Buffer + BufferSize);

	if (m_ulLength)
	{
		memcpy (m_pHead, rNetBuffer.m_pHead, m_ulLength);
	}

	if (m_ulPrivateDataLength)
	{
		assert (m_ulPrivateDataLength <= MaxPrivateData);
		memcpy (m_PrivateData, rNetBuffer.m_PrivateData, m_ulPrivateDataLength);
	}
}

CNetBuffer::~CNetBuffer (void)
{
	assert (m_bValid);
	assert (!m_pNext);	// not enqueued

	m_bValid = FALSE;
}

void *CNetBuffer::GetPtr (void) const
{
	assert (m_bValid);
	assert (m_pHead);

	return m_pHead;
}

size_t CNetBuffer::GetLength (void) const
{
	assert (m_bValid);
	assert (m_pHead);

	return m_ulLength;
}

void *CNetBuffer::AddHeader (size_t ulLength)
{
	assert (m_bValid);
	assert (ulLength);
	assert (m_pHead);

	m_pHead -= ulLength;
	m_ulLength += ulLength;

	assert (m_pHead >= m_Buffer);

	return m_pHead;
}

void *CNetBuffer::RemoveHeader (size_t ulLength)
{
	assert (m_bValid);
	assert (ulLength);
	assert (m_ulLength >= ulLength);
	assert (m_pHead);

	m_pHead += ulLength;
	m_ulLength -= ulLength;

	return m_pHead;
}

void CNetBuffer::RemoveTrailer (size_t ulLength)
{
	assert (m_bValid);
	assert (ulLength);
	assert (m_ulLength > ulLength);
	assert (m_pHead);

	m_ulLength -= ulLength;
}

void CNetBuffer::SetPrivateData (const void *pBuffer, size_t ulLength)
{
	assert (m_bValid);
	assert (pBuffer);
	assert (ulLength);
	assert (ulLength <= MaxPrivateData);

	memcpy (m_PrivateData, pBuffer, ulLength);

	m_ulPrivateDataLength = ulLength;
}

const void *CNetBuffer::GetPrivateData (void) const
{
	assert (m_bValid);

	return m_PrivateData;
}

size_t CNetBuffer::GetPrivateDataLength (void) const
{
	assert (m_bValid);

	return m_ulPrivateDataLength;
}

void CNetBuffer::Dump (const char *pSource)
{
	if (!pSource)
	{
		pSource = From;
	}

	if (!m_bValid)
	{
		CLogger::Get ()->Write (pSource, LogDebug, "Buffer %p is not valid", this);

		return;
	}

	unsigned nSum = 0;
	const u8 *p = m_pHead;
	if (p)
	{
		for (size_t ulLen = m_ulLength; ulLen; ulLen--)
		{
			nSum += *p++;
		}
	}

	CLogger::Get ()->Write (pSource, LogDebug, "Buffer %p (purpose %u, len %lu, sum %x)",
				this, m_Purpose, m_ulLength, nSum);

	if (m_ulLength)
	{
		DebugHexDump (m_pHead, m_ulLength, pSource, DEBUG_HEXDUMP_ASCII);
	}

	if (m_ulPrivateDataLength)
	{
		CLogger::Get ()->Write (pSource, LogDebug, "Private data:");

		DebugHexDump (m_PrivateData, m_ulPrivateDataLength, pSource, 0);
	}

	if (m_pNext)
	{
		CLogger::Get ()->Write (pSource, LogDebug, "Buffer %p is enqueued (%p)",
					this, m_pNext);
	}
}
