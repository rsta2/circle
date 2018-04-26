//
// mqttreceivepacket.h
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
#ifndef _circle_net_mqttreceivepacket_h
#define _circle_net_mqttreceivepacket_h

#include <circle/net/mqtt.h>
#include <circle/net/socket.h>
#include <circle/types.h>

enum TMQTTReceiveStatus
{
	MQTTReceiveContinue,
	MQTTReceivePacketOK,
	MQTTReceivePacketInvalid,
	MQTTReceiveDisconnect,
	MQTTReceiveNotEnoughMemory,
	MQTTReceiveUnknown
};

class CMQTTReceivePacket	/// MQTT helper class
{
public:
	CMQTTReceivePacket (size_t nMaxPacketSize, size_t nMaxPacketsQueued);
	~CMQTTReceivePacket (void);

	void Reset (void);
	void Complete (void);

	TMQTTReceiveStatus Receive (CSocket *pSocket);

	TMQTTPacketType GetType (void) const;
	u8 GetFlags (void) const;
	size_t GetRemainingLength (void) const;

	u8 GetByte (void);
	u16 GetWord (void);
	size_t GetString (char *pBuffer, size_t nBufferSize);
	const u8 *GetData (size_t nLength);

private:
	size_t m_nBufferSize;

	u8 *m_pBuffer;
	unsigned m_nInPtr;
	unsigned m_nOutPtr;

	TMQTTPacketType m_Type;
	u8 m_uchFlags;

	size_t m_nRemainingLength;
	size_t m_nBytesConsumed;
};

#endif
