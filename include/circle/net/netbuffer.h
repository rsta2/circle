//
// netbuffer.h
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
#ifndef _circle_net_netbuffer_h
#define _circle_net_netbuffer_h

#include <circle/netdevice.h>
#include <circle/synchronize.h>
#include <circle/types.h>

class CNetBuffer	// Generic frame/packet buffer for network protocol handling
{
public:
	enum TPurpose		// Defines for what a new net buffer is used for:
	{
		Receive,	// receive path
		TCPSend,	// sending TCP segments (without TCP header option)
		TCPSendMSS,	// sending TCP segments (with MSS TCP header option)
		UDPSend,	// sending UDP packets
		ICMPSend,	// sending ICMP packets
		IGMPSend,	// sending IGMP packets
		ARPSend,	// sending ARP frames (with Ethernet header)
		LLRawSend	// sending raw (IEEE 802.1X EAP) frames from link layer
	};

public:
	// ulLength bytes at pBuffer will be copied into the net buffer (if specified)
	// or ulLength bytes will be reserved for Receive net buffer
	CNetBuffer (TPurpose Purpose, size_t ulLength = 0, const void *pBuffer = nullptr);
	CNetBuffer (const CNetBuffer &rNetBuffer);	// Clone net buffer
	~CNetBuffer (void);

	void *GetPtr (void) const;		// Pointer to current net buffer head
	size_t GetLength (void) const;		// Current length of net buffer

	// Net buffer manipulation
	void *AddHeader (size_t ulLength);	// Add header in front of buffer, return new front
	void *RemoveHeader (size_t ulLength);	// Remove header from front, return new front
	void AddPadding (size_t ulLength);	// Fill with zeros
	void RemoveTrailer (size_t ulLength);	// Remove trailer from back of net buffer

	// Set and get private data of net buffer
	void SetPrivateData (const void *pBuffer, size_t ulLength);
	const void *GetPrivateData (void) const;
	size_t GetPrivateDataLength (void) const;

	void Dump (const char *pSource = nullptr);

private:
	CNetBuffer *m_pNext;			// Is nullptr, if not enqueued
	friend class CNetBufferQueue;

	TPurpose m_Purpose;

	boolean m_bValid;			// for use-after-free detection

	// Additionally reserved space in front to be sure
	static const unsigned HeaderReserve = DATA_CACHE_LINE_LENGTH_MAX;
	static const unsigned BufferSize = FRAME_BUFFER_SIZE + HeaderReserve;
	DMA_BUFFER (u8, m_Buffer, BufferSize);

	u8 *m_pHead;
	size_t m_ulLength;

	static const unsigned MaxPrivateData = 20;
	u8 m_PrivateData[MaxPrivateData];
	size_t m_ulPrivateDataLength;
};

#endif
