//
// retransmissionqueue.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_net_retransmissionqueue_h
#define _circle_net_retransmissionqueue_h

#include <circle/types.h>

class CRetransmissionQueue
{
public:
	CRetransmissionQueue (unsigned nSize);
	~CRetransmissionQueue (void);

	boolean IsEmpty (void) const;
	
	unsigned GetFreeSpace (void) const;
	void Write (const void *pBuffer, unsigned nLength);

	unsigned GetBytesAvailable (void) const;
	void Read (void *pBuffer, unsigned nLength);
	void Advance (unsigned nBytes);
	void Reset (void);

	void Flush (void);

private:
	unsigned m_nSize;

	u8 *m_pBuffer;

	unsigned m_nInPtr;
	unsigned m_nOutPtr;
	unsigned m_nPreOutPtr;
};

#endif
