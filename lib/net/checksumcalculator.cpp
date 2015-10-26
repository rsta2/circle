//
// checksumcalculator.cpp
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
#include <circle/net/checksumcalculator.h>
#include <circle/util.h>
#include <assert.h>

CChecksumCalculator::CChecksumCalculator (const CIPAddress &rSourceIP, int nProtocol)
:	m_bDestAddressSet (FALSE)
{
	rSourceIP.CopyTo (m_Header.SourceAddress);
	m_Header.nZero = 0;
	m_Header.nProtocol = (u8) nProtocol;
}

CChecksumCalculator::CChecksumCalculator (const CIPAddress &rSourceIP,
					  const CIPAddress &rDestIP,
					  int nProtocol)
:	m_bDestAddressSet (TRUE)
{
	rSourceIP.CopyTo (m_Header.SourceAddress);
	rDestIP.CopyTo (m_Header.DestinationAddress);
	m_Header.nZero = 0;
	m_Header.nProtocol = (u8) nProtocol;
}

CChecksumCalculator::~CChecksumCalculator (void)
{
}

void CChecksumCalculator::SetSourceAddress (const CIPAddress &rSourceIP)
{
	rSourceIP.CopyTo (m_Header.SourceAddress);
}

void CChecksumCalculator::SetDestinationAddress (const CIPAddress &rDestIP)
{
	rDestIP.CopyTo (m_Header.DestinationAddress);
	m_bDestAddressSet = TRUE;
}

u16 CChecksumCalculator::Calculate (const void *pBuffer, unsigned nLength)
{
	assert (m_bDestAddressSet);

	m_Header.nTCPLength = le2be16 (nLength);
	u32 nChecksum = CalculateChunk (&m_Header, sizeof m_Header, 0);

	assert (pBuffer != 0);
	assert (nLength > 0);
	nChecksum = CalculateChunk (pBuffer, nLength, nChecksum);

	return ~FoldResult (nChecksum);
}

u16 CChecksumCalculator::SimpleCalculate (const void *pBuffer, unsigned nLength)
{
	assert (pBuffer != 0);
	assert (nLength > 0);
	u32 nChecksum = CalculateChunk (pBuffer, nLength, 0);

	return ~FoldResult (nChecksum);
}

u32 CChecksumCalculator::CalculateChunk (const void *pBuffer, unsigned nLength, u32 nChecksum)
{
	u16 *pBuffer16 = (u16 *) pBuffer;
	assert (pBuffer16 != 0);
	assert (nLength > 0);

	while (nLength >= 2)
	{
		nChecksum += *pBuffer16++;
		nLength -= 2;
	}

	assert (nLength <= 1);
	if (nLength != 0)
	{
		nChecksum += *(u8 *) pBuffer16;
	}
	
	return nChecksum;
}

u16 CChecksumCalculator::FoldResult (u32 nChecksum)
{
	u16 nHigh = nChecksum >> 16;
	while (nHigh != 0)
	{
		nChecksum &= 0xFFFF;
		nChecksum += nHigh;

		nHigh = nChecksum >> 16;
	}
	
	return (u16) nChecksum;
}
