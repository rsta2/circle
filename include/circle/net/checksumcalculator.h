//
// checksumcalculator.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_net_checksumcalculator_h
#define _circle_net_checksumcalculator_h

#include <circle/net/ipaddress.h>
#include <circle/macros.h>
#include <circle/types.h>

#define CHECKSUM_OK	0

struct TPseudoHeader
{
	u8	SourceAddress[IP_ADDRESS_SIZE];
	u8	DestinationAddress[IP_ADDRESS_SIZE];
	u8	nZero;
	u8	nProtocol;
	u16	nTCPLength;	// also for UDP
}
PACKED;

class CChecksumCalculator
{
public:
	CChecksumCalculator (const CIPAddress &rSourceIP, int nProtocol);
	CChecksumCalculator (const CIPAddress &rSourceIP, const CIPAddress &rDestIP, int nProtocol);
	~CChecksumCalculator (void);

	void SetSourceAddress (const CIPAddress &rSourceIP);
	void SetDestinationAddress (const CIPAddress &rDestIP);
	
	u16 Calculate (const void *pBuffer, unsigned nLength);

	static u16 SimpleCalculate (const void *pBuffer, unsigned nLength);

private:
	static u32 CalculateChunk (const void *pBuffer, unsigned nLength, u32 nChecksum);

	static u16 FoldResult (u32 nChecksum);
	
private:
	TPseudoHeader m_Header;
	boolean m_bDestAddressSet;
};

#endif
