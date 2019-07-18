//
// netdevice.h
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
#ifndef _circle_netdevice_h
#define _circle_netdevice_h

#include <circle/macaddress.h>
#include <circle/types.h>

#define FRAME_BUFFER_SIZE	1600

#define MAX_NET_DEVICES		5

enum TNetDeviceSpeed
{
	NetDeviceSpeed10Half,
	NetDeviceSpeed10Full,
	NetDeviceSpeed100Half,
	NetDeviceSpeed100Full,
	NetDeviceSpeed1000Half,
	NetDeviceSpeed1000Full,
	NetDeviceSpeedUnknown
};

class CNetDevice
{
public:
	virtual ~CNetDevice (void) {}

	virtual const CMACAddress *GetMACAddress (void) const = 0;

	virtual boolean SendFrame (const void *pBuffer, unsigned nLength) = 0;

	// pBuffer must have size FRAME_BUFFER_SIZE
	virtual boolean ReceiveFrame (void *pBuffer, unsigned *pResultLength) = 0;

	// returns TRUE if PHY link is up
	virtual boolean IsLinkUp (void)			{ return TRUE; }

	virtual TNetDeviceSpeed GetLinkSpeed (void)	{ return NetDeviceSpeedUnknown; }

	static const char *GetSpeedString (TNetDeviceSpeed Speed);

	// nDeviceNumber is 0-based
	static CNetDevice *GetNetDevice (unsigned nDeviceNumber);

protected:
	void AddNetDevice (void);

private:
	static unsigned s_nDeviceNumber;
	static CNetDevice *s_pDevice[MAX_NET_DEVICES];

	static const char *s_SpeedString[NetDeviceSpeedUnknown];
};

#endif
