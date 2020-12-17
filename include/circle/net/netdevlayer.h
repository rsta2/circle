//
// netdevlayer.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2020  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_net_netdevlayer_h
#define _circle_net_netdevlayer_h

#include <circle/net/netconfig.h>
#include <circle/netdevice.h>
#include <circle/net/netqueue.h>
#include <circle/bcm54213.h>
#include <circle/types.h>

class CNetDeviceLayer
{
public:
	CNetDeviceLayer (CNetConfig *pNetConfig, TNetDeviceType DeviceType);
	~CNetDeviceLayer (void);

	boolean Initialize (boolean bWaitForActivate);

	void Process (void);

	// returns 0, if net device is not available yet
	const CMACAddress *GetMACAddress (void) const;

	void Send (const void *pBuffer, unsigned nLength);
	boolean Receive (void *pBuffer, unsigned *pResultLength);

	boolean IsRunning (void) const;			// is net device available?

private:
	TNetDeviceType m_DeviceType;
	CNetConfig *m_pNetConfig;
	CNetDevice *m_pDevice;

	CNetQueue m_TxQueue;
	CNetQueue m_RxQueue;

#if RASPPI >= 4
	CBcm54213Device m_Bcm54213;
#endif
};

#endif
