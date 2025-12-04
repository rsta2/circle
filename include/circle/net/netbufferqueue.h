//
// netbufferqueue.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2025  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_net_netbufferqueue_h
#define _circle_net_netbufferqueue_h

#include <circle/net/netbuffer.h>
#include <circle/spinlock.h>
#include <circle/types.h>

class CNetBufferQueue		// FIFO queue for CNetBuffer objects
{
public:
	CNetBufferQueue (boolean bProtected = FALSE);	// Uses spin lock, if protected
	~CNetBufferQueue (void);

	boolean IsEmpty (void) const;

	void Flush (void);

	void Enqueue (CNetBuffer *pNetBuffer);

	// returns nullptr if queue is empty
	CNetBuffer *Dequeue (void);

	// returns first entry without dequeuing
	CNetBuffer *Peek (void) const;

private:
	CNetBuffer *volatile m_pFirst;
	CNetBuffer *volatile m_pLast;

	boolean m_bProtected;
	CSpinLock m_SpinLock;
};

#endif
