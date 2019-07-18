//
// bcm54213.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2019  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_bcm54213_h
#define _circle_bcm54213_h

#include <circle/netdevice.h>
#include <circle/macaddress.h>
#include <circle/types.h>

class CBcm54213Device : public CNetDevice
{
public:
	CBcm54213Device (void);
	~CBcm54213Device (void);

	boolean Initialize (void);

	const CMACAddress *GetMACAddress (void) const;

	boolean SendFrame (const void *pBuffer, unsigned nLength);

	// pBuffer must have size FRAME_BUFFER_SIZE
	boolean ReceiveFrame (void *pBuffer, unsigned *pResultLength);

	// returns TRUE if PHY link is up
	boolean IsLinkUp (void);

	TNetDeviceSpeed GetLinkSpeed (void);

private:
	CMACAddress m_MACAddress;
};

#endif
