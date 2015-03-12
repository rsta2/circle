//
// devicenameservice.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2015  R. Stange <rsta2@o2online.de>
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
#ifndef _devicenameservice_h
#define _devicenameservice_h

#include <circle/device.h>
#include <circle/spinlock.h>
#include <circle/types.h>

struct TDeviceInfo
{
	TDeviceInfo	*pNext;
	char		*pName;
	CDevice		*pDevice;
	boolean		 bBlockDevice;
};

class CDeviceNameService
{
public:
	CDeviceNameService (void);
	~CDeviceNameService (void);

	void AddDevice (const char *pName, CDevice *pDevice, boolean bBlockDevice);

	CDevice *GetDevice (const char *pName, boolean bBlockDevice);
	
	static CDeviceNameService *Get (void);

private:
	TDeviceInfo *m_pList;

	CSpinLock m_SpinLock;

	static CDeviceNameService *s_This;
};

#endif
