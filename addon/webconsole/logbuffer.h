//
// logbuffer.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2016  R. Stange <rsta2@o2online.de>
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
#ifndef _webconsole_logbuffer_h
#define _webconsole_logbuffer_h

#include <circle/types.h>

class CLogBuffer
{
public:
	CLogBuffer (unsigned nSize);
	~CLogBuffer (void);

	void Put (const void *pBuffer, unsigned nLength);

	unsigned Get (void *pBuffer);

private:
	unsigned m_nSize;

	u8 *m_pBuffer;

	unsigned m_nInPtr;
	unsigned m_nOutPtr;
};

#endif
