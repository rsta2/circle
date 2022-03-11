//
// queue.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2021  R. Stange <rsta2@o2online.de>
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
#ifndef _queue_h
#define _queue_h

#include <circle/types.h>

class CQueue
{
public:
	CQueue (unsigned nSize);
	~CQueue (void);

	boolean IsEmpty (void) const;
	
	unsigned GetFreeSpace (void) const;
	void Write (const void *pBuffer, unsigned nLength);

	unsigned GetBytesAvailable (void) const;
	void Read (void *pBuffer, unsigned nLength);

	void Flush (void);

private:
	unsigned m_nSize;

	u8 *m_pBuffer;

	unsigned m_nInPtr;
	unsigned m_nOutPtr;
};

#endif
