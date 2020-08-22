//
// dnsclient.cpp
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
#include <circle/net/dnsclient.h>
#include <circle/net/socket.h>
#include <circle/net/in.h>
#include <circle/sched/scheduler.h>
#include <circle/macros.h>
#include <circle/util.h>
#include <assert.h>

#define MAX_HOSTNAME_SIZE	256
#define DNS_MAX_MESSAGE_SIZE	512

struct TDNSHeader
{
	unsigned short nID;
	unsigned short nFlags;
#define DNS_FLAGS_QR		0x8000
#define DNS_FLAGS_OPCODE	0x7800
	#define DNS_FLAGS_OPCODE_QUERY		0x0000
	#define DNS_FLAGS_OPCODE_IQUERY		0x0800
	#define DNS_FLAGS_OPCODE_STATUS		0x1000
#define DNS_FLAGS_AA		0x0400
#define DNS_FLAGS_TC		0x0200
#define DNS_FLAGS_RD		0x0100
#define DNS_FLAGS_RA		0x0080
#define DNS_FLAGS_RCODE		0x000F
	#define DNS_RCODE_SUCCESS		0x0000
	#define DNS_RCODE_FORMAT_ERROR		0x0001
	#define DNS_RCODE_SERVER_FAILURE	0x0002
	#define DNS_RCODE_NAME_ERROR		0x0003
	#define DNS_RCODE_NOT_IMPLEMENTED	0x0004
	#define DNS_RCODE_REFUSED		0x0005
	unsigned short nQDCount;
	unsigned short nANCount;
	unsigned short nNSCount;
	unsigned short nARCount;
}
PACKED;

struct TDNSQueryTrailer
{
	unsigned short nQType;
#define DNS_QTYPE_A		1
	unsigned short nQClass;
#define DNS_QCLASS_IN		1
}
PACKED;

struct TDNSResourceRecordTrailerAIN
{
	unsigned short nType;
	unsigned short nClass;
	unsigned int   nTTL;
	unsigned short nRDLength;
#define DNS_RDLENGTH_AIN	4
	unsigned char  RData[DNS_RDLENGTH_AIN];
}
PACKED;

#define DNS_RR_TRAILER_HEADER_LENGTH	( sizeof (struct TDNSResourceRecordTrailerAIN) \
					 - DNS_RDLENGTH_AIN)

u16 CDNSClient::s_nXID = 1;

CDNSClient::CDNSClient (CNetSubSystem *pNetSubSystem)
:	m_pNetSubSystem (pNetSubSystem)
{
	assert (m_pNetSubSystem != 0);
}

CDNSClient::~CDNSClient (void)
{
	m_pNetSubSystem = 0;
}

boolean CDNSClient::Resolve (const char *pHostname, CIPAddress *pIPAddress)
{
	assert (pHostname != 0);

	if ('1' <= *pHostname && *pHostname <= '9')
	{
		if (ConvertIPString (pHostname, pIPAddress))
		{
			return TRUE;
		}
	}

	assert (m_pNetSubSystem != 0);
	CIPAddress DNSServer (m_pNetSubSystem->GetConfig ()->GetDNSServer ()->Get ());
	if (DNSServer.IsNull ())
	{
		return FALSE;
	}

	CSocket Socket (m_pNetSubSystem, IPPROTO_UDP);
	if (Socket.Connect (DNSServer, 53) != 0)
	{
		return FALSE;
	}

	u8 Buffer[DNS_MAX_MESSAGE_SIZE];
	memset (Buffer, 0, sizeof Buffer);
	TDNSHeader *pDNSHeader = (TDNSHeader *) Buffer;

	u16 nXID = s_nXID++;

	pDNSHeader->nID      = le2be16 (nXID);
	pDNSHeader->nFlags   = BE (DNS_FLAGS_OPCODE_QUERY | DNS_FLAGS_RD);
	pDNSHeader->nQDCount = BE (1);

	u8 *pQuery = Buffer + sizeof (TDNSHeader);

	char Hostname[MAX_HOSTNAME_SIZE];
	strncpy (Hostname, pHostname, MAX_HOSTNAME_SIZE-1);
	Hostname[MAX_HOSTNAME_SIZE-1] = '\0';

	char *pSavePtr;
	size_t nLength;
	char *pLabel = strtok_r (Hostname, ".", &pSavePtr);
	while (pLabel != 0)
	{
		nLength = strlen (pLabel);
		if (   nLength > 255
		    || (int) (nLength+1+1) >= DNS_MAX_MESSAGE_SIZE-(pQuery-Buffer))
		{
			return FALSE;
		}

		*pQuery++ = (u8) nLength;

		strcpy ((char *) pQuery, pLabel);
		pQuery += nLength;

		pLabel = strtok_r (0, ".", &pSavePtr);
	}

	*pQuery++ = '\0';

	TDNSQueryTrailer QueryTrailer;
	QueryTrailer.nQType  = BE (DNS_QTYPE_A);
	QueryTrailer.nQClass = BE (DNS_QCLASS_IN);

	if ((int) (sizeof QueryTrailer) > DNS_MAX_MESSAGE_SIZE-(pQuery-Buffer))
	{
		return FALSE;
	}
	memcpy (pQuery, &QueryTrailer, sizeof QueryTrailer);
	pQuery += sizeof QueryTrailer;
	
	int nSize = pQuery - Buffer;
	assert (nSize <= DNS_MAX_MESSAGE_SIZE);

	unsigned char RecvBuffer[DNS_MAX_MESSAGE_SIZE];
	int nRecvSize;

	unsigned nTry = 1;
	do
	{
		if (   nTry++ > 3
		    || Socket.Send (Buffer, nSize, 0) != nSize)
		{
			return FALSE;
		}

		CScheduler::Get ()->MsSleep (1000);

		nRecvSize = Socket.Receive (RecvBuffer, DNS_MAX_MESSAGE_SIZE, MSG_DONTWAIT);
		assert (nRecvSize < DNS_MAX_MESSAGE_SIZE);
		if (nRecvSize < 0)
		{
			return FALSE;
		}
	}
	while (nRecvSize < (int) (sizeof (TDNSHeader)+sizeof (TDNSResourceRecordTrailerAIN)));

	pDNSHeader = (TDNSHeader *) RecvBuffer;
	if (   pDNSHeader->nID != le2be16 (nXID)
	    ||    (pDNSHeader->nFlags & BE (  DNS_FLAGS_QR
	                                    | DNS_FLAGS_OPCODE
	                                    | DNS_FLAGS_TC
	                                    | DNS_FLAGS_RCODE))
	       != BE (DNS_FLAGS_QR | DNS_FLAGS_OPCODE_QUERY | DNS_RCODE_SUCCESS)
	    || pDNSHeader->nQDCount != BE (1)
	    || pDNSHeader->nANCount == BE (0))
	{
		return FALSE;
	}

	u8 *pResponse = RecvBuffer + sizeof (TDNSHeader);

	// parse the query section
	while ((nLength = *pResponse++) > 0)
	{
		pResponse += nLength;
		if (pResponse-RecvBuffer >= nRecvSize)
		{
			return FALSE;
		}
	}

	pResponse += sizeof (TDNSQueryTrailer);
	if (pResponse-RecvBuffer >= nRecvSize)
	{
		return FALSE;
	}

	TDNSResourceRecordTrailerAIN RRTrailer;

	// parse the answer section
	while (1)
	{
		nLength = *pResponse++;
		if ((nLength & 0xC0) == 0xC0)		// check for compression
		{
			pResponse++;
		}
		else
		{
			do
			{
				pResponse += nLength;
				if (pResponse-RecvBuffer >= nRecvSize)
				{
					return FALSE;
				}
			}
			while ((nLength = *pResponse++) > 0);
		}

		if (pResponse-RecvBuffer > (int) (nRecvSize-sizeof RRTrailer))
		{
			return FALSE;
		}

		memcpy (&RRTrailer, pResponse, sizeof RRTrailer);

		if (   RRTrailer.nType     == BE (DNS_QTYPE_A)
		    && RRTrailer.nClass    == BE (DNS_QCLASS_IN)
		    && RRTrailer.nRDLength == BE (DNS_RDLENGTH_AIN))
		{
			break;
		}

		pResponse += DNS_RR_TRAILER_HEADER_LENGTH + BE (RRTrailer.nRDLength);
		if (pResponse-RecvBuffer >= nRecvSize)
		{
			return FALSE;
		}
	}

	assert (pIPAddress != 0);
	pIPAddress->Set (RRTrailer.RData);

	return TRUE;
}

boolean CDNSClient::ConvertIPString (const char *pIPString, CIPAddress *pIPAddress)
{
	u8 IPAddress[IP_ADDRESS_SIZE];

	for (unsigned i = 0; i <= 3; i++)
	{
		char *pEnd = 0;
		assert (pIPString != 0);
		unsigned long nNumber = strtoul (pIPString, &pEnd, 10);

		if (i < 3)
		{
			if (    pEnd == 0
			    || *pEnd != '.')
			{
				return FALSE;
			}
		}
		else
		{
			if (    pEnd != 0
			    && *pEnd != '\0')
			{
				return FALSE;
			}
		}

		if (nNumber > 255)
		{
			return FALSE;
		}

		IPAddress[i] = (u8) nNumber;

		assert (pEnd != 0);
		pIPString = pEnd + 1;
	}

	assert (pIPAddress != 0);
	pIPAddress->Set (IPAddress);

	return TRUE;
}
