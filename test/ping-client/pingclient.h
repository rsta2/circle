//
// pingclient.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2024  R. Stange <rsta2@o2online.de>
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
#ifndef _pingclient_h
#define _pingclient_h

#include <circle/net/netsubsystem.h>
#include <circle/net/networklayer.h>
#include <circle/net/ipaddress.h>
#include <circle/types.h>

class CPingClient	/// ICMP echo (ping) client
{
public:
	/// \param pNet Pointer to the network subsystem object
	CPingClient (CNetSubSystem *pNet);

	~CPingClient (void);

	/// \param rServer IP address of the ping server
	/// \param usIdentifier ID of the echo packet (1 .. N)
	/// \param usSequenceNumber Sequence number of the echo packet (1 .. N)
	/// \param ulLength Length of echo packet (> 8, default 64)
	/// \return Operation successful?
	boolean Send (const CIPAddress &rServer,
		      u16 usIdentifier, u16 usSequenceNumber,
		      size_t ulLength = 64);

	/// \param pSender Object, which is set to the IP address of the sender
	/// \param pIdentifier Variable, which is set to the ID of the echo packet
	/// \param pSequenceNumber Variable, which is set to the sequence number of the echo packet
	/// \param pLength Variable, which is set to the length of the echo packet
	/// \return ICMP echo (ping) reply received?
	boolean Receive (CIPAddress *pSender,
			 u16 *pIdentifier, u16 *pSequenceNumber,
			 size_t *pLength);

private:
	CNetworkLayer *m_pNetworkLayer;

	static unsigned s_nInstanceCount;
};

#endif
