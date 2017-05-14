//
// netdevice.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2017  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/netdevice.h>
#include <circle/devicenameservice.h>
#include <circle/string.h>

unsigned CNetDevice::s_nDeviceNumber = 0;

CNetDevice::CNetDevice (CUSBFunction *pFunction)
:	CUSBFunction (pFunction)
{
}

CNetDevice::~CNetDevice (void)
{
}

void CNetDevice::AddNetDevice (void)
{
	CString DeviceName;
	DeviceName.Format ("eth%u", s_nDeviceNumber++);
	CDeviceNameService::Get ()->AddDevice (DeviceName, this, FALSE);
}
