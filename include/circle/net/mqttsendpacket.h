//
// mqttsendpacket.h
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
#ifndef _circle_net_mqttsendpacket_h
#define _circle_net_mqttsendpacket_h

#include <circle/net/mqtt.h>
#include <circle/net/socket.h>
#include <circle/types.h>

class CMQTTSendPacket		/// MQTT helper class
{
public:
	CMQTTSendPacket (TMQTTPacketType Type, size_t nMaxPacketSize = 128);
	~CMQTTSendPacket (void);

	void SetFlags (u8 uchFlags);

	void AppendByte (u8 uchValue);
	void AppendWord (u16 usValue);		// usValue is little endian
	void AppendString (const char *pString);
	void AppendData (const u8 *pBuffer, size_t nLength);

	boolean Send (CSocket *pSocket);

	TMQTTPacketType GetType (void) const;
	u8 GetFlags (void) const;

	// for packet retransmission queue:

	void SetScheduledTime (unsigned nTime);
	unsigned GetScheduledTime (void) const;

	void SetQoS (u8 uchQoS);
	u8 GetQoS (void) const;

	void SetPacketIdentifier (u16 usPacketIdentifier);
	u16 GetPacketIdentifier (void) const;

private:
	TMQTTPacketType m_Type;
	size_t m_nMaxPacketSize;

	boolean m_bError;

	u8 *m_pBuffer;
	unsigned m_nBufPtr;

	u8 m_uchFlags;

	unsigned m_nSendTries;

	// for packet retransmission queue
	unsigned m_nScheduledTime;
	u8 m_uchQoS;
	u16 m_usPacketIdentifier;
};

#endif
