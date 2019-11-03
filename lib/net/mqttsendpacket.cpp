//
// mqttsendpacket.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2018  R. Stange <rsta2@o2online.de>
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
#include <circle/net/mqttsendpacket.h>
#include <circle/net/in.h>
#include <circle/util.h>
#include <assert.h>

#define MAX_LENGTH_FIXED_HEADER		5

CMQTTSendPacket::CMQTTSendPacket (TMQTTPacketType Type, size_t nMaxPacketSize)
:	m_Type (Type),
	m_nMaxPacketSize (nMaxPacketSize),
	m_bError (FALSE),
	m_nBufPtr (MAX_LENGTH_FIXED_HEADER),
	m_uchFlags (0),
	m_nSendTries (MQTT_SEND_TRIES)
{
	assert (m_nMaxPacketSize >= 128);
	m_pBuffer = new u8[m_nMaxPacketSize];
	if (m_pBuffer == 0)
	{
		m_bError = TRUE;

		return;
	}

	if (   m_Type == MQTTPubRel
	    || m_Type == MQTTSubscribe
	    || m_Type == MQTTUnsubscribe)
	{
		m_uchFlags = 1 << 1;
	}
}

CMQTTSendPacket::~CMQTTSendPacket (void)
{
	delete [] m_pBuffer;
	m_pBuffer = 0;
}

void CMQTTSendPacket::SetFlags (u8 uchFlags)
{
	assert (m_Type == MQTTPublish);
	assert (uchFlags <= 0x0F);
	m_uchFlags = uchFlags;
}

void CMQTTSendPacket::AppendByte (u8 uchValue)
{
	if (m_bError)
	{
		return;
	}

	if (m_nBufPtr >= m_nMaxPacketSize)
	{
		m_bError = TRUE;

		return;
	}

	assert (m_pBuffer != 0);
	m_pBuffer[m_nBufPtr++] = uchValue;
}

void CMQTTSendPacket::AppendWord (u16 usValue)
{
	if (m_bError)
	{
		return;
	}

	if (m_nBufPtr+1 >= m_nMaxPacketSize)
	{
		m_bError = TRUE;

		return;
	}

	assert (m_pBuffer != 0);
	m_pBuffer[m_nBufPtr++] = usValue >> 8;
	m_pBuffer[m_nBufPtr++] = usValue & 0xFF;
}

void CMQTTSendPacket::AppendString (const char *pString)	// TODO: support characters > '\x7F'
{
	if (m_bError)
	{
		return;
	}

	assert (pString != 0);
	size_t nLength = strlen (pString);
	if (m_nBufPtr+nLength+2 > m_nMaxPacketSize)
	{
		m_bError = TRUE;

		return;
	}

	m_pBuffer[m_nBufPtr++] = nLength >> 8;
	m_pBuffer[m_nBufPtr++] = nLength & 0xFF;

	if (nLength > 0)
	{
		memcpy (m_pBuffer+m_nBufPtr, pString, nLength);
		m_nBufPtr += nLength;
	}
}

void CMQTTSendPacket::AppendData (const u8 *pBuffer, size_t nLength)
{
	if (m_bError)
	{
		return;
	}

	if (m_nBufPtr+nLength > m_nMaxPacketSize)
	{
		m_bError = TRUE;

		return;
	}

	if (nLength > 0)
	{
		assert (pBuffer != 0);
		memcpy (m_pBuffer+m_nBufPtr, pBuffer, nLength);
		m_nBufPtr += nLength;
	}
}

boolean CMQTTSendPacket::Send (CSocket *pSocket)
{
	if (m_bError)
	{
		return FALSE;
	}

	if (m_nSendTries == 0)
	{
		return FALSE;
	}
	m_nSendTries--;

	// calculate and encode remaining length
	assert (m_nBufPtr >= MAX_LENGTH_FIXED_HEADER);
	unsigned nRemainingLength = m_nBufPtr-MAX_LENGTH_FIXED_HEADER;

	u8 EncodedLength[4];
	unsigned nTempLength = nRemainingLength;
	unsigned nLengthBytes = 0;
	do
	{
		u8 uchByte = nTempLength & 0x7F;

		nTempLength >>= 7;
		if (nTempLength > 0)
		{
			uchByte |= 0x80;
		}

		assert (nLengthBytes < 4);
		EncodedLength[nLengthBytes++] = uchByte;
	}
	while (nTempLength > 0);

	// insert remaining length
	assert (nLengthBytes > 0);
	assert (nLengthBytes < MAX_LENGTH_FIXED_HEADER-1);
	assert (m_pBuffer != 0);
	for (unsigned i = 0; i < nLengthBytes; i++)
	{
		m_pBuffer[MAX_LENGTH_FIXED_HEADER-nLengthBytes+i] = EncodedLength[i];
	}

	// insert control byte
	m_pBuffer[MAX_LENGTH_FIXED_HEADER-nLengthBytes-1] = ((u8) m_Type << 4) | m_uchFlags;

	assert (pSocket != 0);
	int nSendLength = 1+nLengthBytes+nRemainingLength;
	if (pSocket->Send (&m_pBuffer[MAX_LENGTH_FIXED_HEADER-nLengthBytes-1],
			   nSendLength, MSG_DONTWAIT) != nSendLength)
	{
		return FALSE;
	}

	return TRUE;
}

TMQTTPacketType CMQTTSendPacket::GetType (void) const
{
	return m_Type;
}

u8 CMQTTSendPacket::GetFlags (void) const
{
	return m_uchFlags;
}

void CMQTTSendPacket::SetScheduledTime (unsigned nTime)
{
	m_nScheduledTime = nTime;
}

unsigned CMQTTSendPacket::GetScheduledTime (void) const
{
	return m_nScheduledTime;
}

void CMQTTSendPacket::SetQoS (u8 uchQoS)
{
	m_uchQoS = uchQoS;
}

u8 CMQTTSendPacket::GetQoS (void) const
{
	return m_uchQoS;
}

void CMQTTSendPacket::SetPacketIdentifier (u16 usPacketIdentifier)
{
	m_usPacketIdentifier = usPacketIdentifier;
}

u16 CMQTTSendPacket::GetPacketIdentifier (void) const
{
	return m_usPacketIdentifier;
}
