//
// netdevice.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
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
#include <circle/netdevice.h>

const char *CNetDevice::s_SpeedString[NetDeviceSpeedUnknown] =
{
	"10BASE-T Half-duplex",
	"10BASE-T Full-duplex",
	"100BASE-TX Half-duplex",
	"100BASE-TX Full-duplex",
	"1000BASE-T Half-duplex",
	"1000BASE-T Full-duplex"
};

unsigned CNetDevice::s_nDeviceNumber = 0;

CNetDevice *CNetDevice::s_pDevice[MAX_NET_DEVICES];

void CNetDevice::AddNetDevice (void)
{
	if (s_nDeviceNumber < MAX_NET_DEVICES)
	{
		s_pDevice[s_nDeviceNumber++] = this;
	}
}

const char *CNetDevice::GetSpeedString (TNetDeviceSpeed Speed)
{
	if (Speed >= NetDeviceSpeedUnknown)
	{
		return "Unknown";
	}

	return s_SpeedString[Speed];
}

CNetDevice *CNetDevice::GetNetDevice (unsigned nDeviceNumber)
{
	if (nDeviceNumber < s_nDeviceNumber)
	{
		return s_pDevice[nDeviceNumber];
	}

	return 0;
}

CNetDevice *CNetDevice::GetNetDevice (TNetDeviceType Type)
{
	for (unsigned nDeviceNumber = 0; nDeviceNumber < s_nDeviceNumber; nDeviceNumber++)
	{
		CNetDevice *pDevice = s_pDevice[nDeviceNumber];
		if (pDevice == 0)
		{
			break;
		}

		if (   Type == NetDeviceTypeAny
		    || pDevice->GetType () == Type)
		{
			return pDevice;
		}
	}

	return 0;
}
