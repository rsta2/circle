//
// mqttreceivepacket.cpp
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
#include <circle/net/mqttreceivepacket.h>
#include <circle/net/in.h>

CMQTTReceivePacket::CMQTTReceivePacket (size_t nMaxPacketSize, size_t nMaxPacketsQueued)
:	m_nBufferSize (nMaxPacketSize*nMaxPacketsQueued + FRAME_BUFFER_SIZE),
	m_nInPtr (0)
{
	assert (nMaxPacketSize >= 128);
	assert (nMaxPacketsQueued >= 1);
	m_pBuffer = new u8[m_nBufferSize];
}

CMQTTReceivePacket::~CMQTTReceivePacket (void)
{
	delete [] m_pBuffer;
	m_pBuffer = 0;
}

void CMQTTReceivePacket::Reset (void)
{
	m_nInPtr = 0;
}

void CMQTTReceivePacket::Complete (void)
{
	assert (m_nInPtr >= m_nOutPtr);

	// another packet may follow, move it in front of the buffer
	unsigned i;
	for (i = 0; m_nInPtr > m_nOutPtr; i++, m_nOutPtr++)
	{
		m_pBuffer[i] = m_pBuffer[m_nOutPtr];
	}

	m_nInPtr = i;
}

TMQTTReceiveStatus CMQTTReceivePacket::Receive (CSocket *pSocket)
{
	if (m_pBuffer == 0)
	{
		return MQTTReceiveNotEnoughMemory;
	}

	if (m_nInPtr > m_nBufferSize-FRAME_BUFFER_SIZE)
	{
		return MQTTReceiveNotEnoughMemory;
	}

	assert (pSocket != 0);
	int nBytesReceived = pSocket->Receive (m_pBuffer+m_nInPtr, FRAME_BUFFER_SIZE, MSG_DONTWAIT);
	if (nBytesReceived < 0)
	{
		return MQTTReceiveDisconnect;
	}

	m_nInPtr += nBytesReceived;
	m_nOutPtr = 1;

	// decode remaining length
	unsigned nShift = 0;
	size_t nRemainingLength = 0;
	u8 uchByte;
	do
	{
		if (nShift > 7*3)
		{
			return MQTTReceivePacketInvalid;
		}

		if (m_nInPtr <= m_nOutPtr)
		{
			return MQTTReceiveContinue;
		}

		uchByte = m_pBuffer[m_nOutPtr];
		if (   uchByte == 0
		    && m_nOutPtr > 1)
		{
			return MQTTReceivePacketInvalid;
		}

		nRemainingLength += (uchByte & 0x7F) << nShift;

		nShift += 7;
		m_nOutPtr++;
	}
	while (uchByte & 0x80);

	if (m_nInPtr < m_nOutPtr+nRemainingLength)
	{
		return MQTTReceiveContinue;
	}

	// packet complete

	m_Type = (TMQTTPacketType) (m_pBuffer[0] >> 4);
	m_uchFlags = m_pBuffer[0] & 0x0F;

	m_nRemainingLength = nRemainingLength;
	m_nBytesConsumed = 0;

	// do some basic checks on the received packet
	switch (m_Type)
	{
	case MQTTConnAck:
		if (   m_uchFlags != 0
		    || nRemainingLength != 2)
		{
			return MQTTReceivePacketInvalid;
		}
		break;

	case MQTTPublish: {
		u8 uchQoS = (m_uchFlags & MQTT_FLAG_QOS__MASK) >> MQTT_FLAG_QOS__SHIFT;
		if (   uchQoS > MQTT_QOS_EXACTLY_ONCE
		    || nRemainingLength < (uchQoS == 0 ? 3 : 5))
		{
			return MQTTReceivePacketInvalid;
		}
		} break;

	case MQTTPubAck:
	case MQTTPubRec:
	case MQTTPubComp:
	case MQTTUnsubAck:
		if (   m_uchFlags != 0
		    || nRemainingLength != 2)
		{
			return MQTTReceivePacketInvalid;
		}
		break;

	case MQTTPubRel:
		if (   m_uchFlags != 1 << 1
		    || nRemainingLength != 2)
		{
			return MQTTReceivePacketInvalid;
		}
		break;

	case MQTTSubAck:
		if (   m_uchFlags != 0
		    || nRemainingLength < 3)
		{
			return MQTTReceivePacketInvalid;
		}
		break;

	case MQTTPingResp:
		if (   m_uchFlags != 0
		    || nRemainingLength != 0)
		{
			return MQTTReceivePacketInvalid;
		}
		break;

	default:
		return MQTTReceivePacketInvalid;
	}

	return MQTTReceivePacketOK;
}

TMQTTPacketType CMQTTReceivePacket::GetType (void) const
{
	return m_Type;
}

u8 CMQTTReceivePacket::GetFlags (void) const
{
	return m_uchFlags;
}

size_t CMQTTReceivePacket::GetRemainingLength (void) const
{
	return m_nRemainingLength;
}

u8 CMQTTReceivePacket::GetByte (void)
{
	m_nBytesConsumed++;
	assert (m_nRemainingLength >= m_nBytesConsumed);

	return m_pBuffer[m_nOutPtr++];
}

u16 CMQTTReceivePacket::GetWord (void)
{
	m_nBytesConsumed += 2;
	assert (m_nRemainingLength >= m_nBytesConsumed);

	u16 usResult = m_pBuffer[m_nOutPtr++] << 8;
	usResult |= m_pBuffer[m_nOutPtr++];

	return usResult;
}

size_t CMQTTReceivePacket::GetString (char *pBuffer, size_t nBufferSize)
{
	u16 usLength = GetWord ();

	m_nBytesConsumed += usLength;
	if (m_nRemainingLength < m_nBytesConsumed)
	{
		return 0;
	}

	if (usLength+1U > nBufferSize)
	{
		return 0;
	}

	for (unsigned i = 0; i < usLength; i++)
	{
		char chChar = (char) m_pBuffer[m_nOutPtr++];
		if (chChar & 0x80)		// TODO: support multi-byte characters
		{
			return 0;
		}

		*pBuffer++ = chChar;
	}

	*pBuffer = '\0';

	return usLength;
}

const u8 *CMQTTReceivePacket::GetData (size_t nLength)
{
	m_nBytesConsumed += nLength;
	if (m_nRemainingLength < m_nBytesConsumed)
	{
		return 0;
	}

	const u8 *pResult = &m_pBuffer[m_nOutPtr];
	m_nOutPtr += nLength;

	return pResult;
}
