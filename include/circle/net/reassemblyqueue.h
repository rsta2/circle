//
// reassemblyqueue.h
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
#ifndef _circle_net_reassemblyqueue_h
#define _circle_net_reassemblyqueue_h

#include <circle/net/netbuffer.h>
#include <circle/net/netbufferqueue.h>
#include <circle/ptrlist.h>
#include <circle/types.h>

class CReassemblyQueue		// Reassembly queue for the TCP receiver
{
public:
	CReassemblyQueue (CNetBufferQueue *pRxQueue, size_t ulMaxBytes);
	~CReassemblyQueue (void);

	// returns TRUE, if packet was enqueued
	boolean Enqueue (u32 nSEG_SEQ, CNetBuffer *pPacket);

	// returns new RCV.NXT
	u32 Dequeue (u32 nRCV_NXT);

	void Flush (void);

private:
	CNetBufferQueue *m_pRxQueue;

	size_t m_ulMaxBytes;
	size_t m_ulCurrentBytes;

	boolean m_bEnabled;

	CPtrList m_List;
};

#endif
