//
// device.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2019  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_device_h
#define _circle_device_h

#include <circle/types.h>

class CDevice
{
public:
	CDevice (void);
	virtual ~CDevice (void);

	// returns number of read bytes or < 0 on failure
	virtual int Read (void *pBuffer, size_t nCount);

	// returns number of written bytes or < 0 on failure
	virtual int Write (const void *pBuffer, size_t nCount);

	// returns the resulting offset, (u64) -1 on error
	virtual u64 Seek (u64 ullOffset);		// byte offset

	// returns TRUE on successful removal
	virtual boolean RemoveDevice (void);
};

#endif
