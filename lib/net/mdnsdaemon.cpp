//
// mdnsdaemon.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2025  R. Stange <rsta2@gmx.net>
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
#include <circle/net/mdnsdaemon.h>
#include <circle/net/in.h>
#include <circle/sched/scheduler.h>
#include <circle/logger.h>
#include <circle/macros.h>
#include <circle/debug.h>
#include <circle/util.h>
#include <assert.h>

#define MDNS_HOST_GROUP		{224, 0, 0, 251}
#define MDNS_PORT		5353
#define MDNS_DOMAIN		"local"

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
#define MDNS_QTYPE_ANY		255
	unsigned short nQClass;
#define DNS_QCLASS_IN		1
#define DNS_QCLASS_CACHE_FLUSH	0x8000
}
PACKED;

struct TDNSResourceRecordTrailerAIN
{
	unsigned short nType;
	unsigned short nClass;
	unsigned int   nTTL;
#define DNS_TTL_DEFAULT		240
	unsigned short nRDLength;
#define DNS_RDLENGTH_AIN	4
	unsigned char  RData[DNS_RDLENGTH_AIN];
}
PACKED;

#define DNS_RR_TRAILER_HEADER_LENGTH	( sizeof (struct TDNSResourceRecordTrailerAIN) \
					 - DNS_RDLENGTH_AIN)

LOGMODULE ("mdnsd");

CmDNSDaemon *CmDNSDaemon::s_pThis = nullptr;

CmDNSDaemon::CmDNSDaemon (CNetSubSystem *pNet)
:	m_pNet (pNet),
	m_bRunning (FALSE),
	m_nSuffix (1),
	m_pSocket (nullptr),
	m_usXID (1)
{
	assert (!s_pThis);
	s_pThis = this;

	SetName ("mdnsd");

	assert (m_pNet);
	m_Hostname = m_pNet->GetHostname ();

	m_HostnameWithDomain = m_Hostname;
	m_HostnameWithDomain.Append ("." MDNS_DOMAIN);
}

CmDNSDaemon::~CmDNSDaemon (void)
{
	delete m_pSocket;
	m_pSocket = nullptr;

	m_pNet = nullptr;

	s_pThis = nullptr;
}

boolean CmDNSDaemon::IsRunning (void) const
{
	return m_bRunning;
}

CString CmDNSDaemon::GetHostname (void) const
{
	return m_Hostname;
}

unsigned CmDNSDaemon::GetSuffix (void) const
{
	return m_nSuffix;
}

void CmDNSDaemon::Run (void)
{
	static const u8 mDNSIPAddress[] = MDNS_HOST_GROUP;
	CIPAddress mDNSIP (mDNSIPAddress);

	assert (!m_pSocket);
	assert (m_pNet);
	m_pSocket = new CSocket (m_pNet, IPPROTO_UDP);
	assert (m_pSocket);
	if (m_pSocket->Bind (MDNS_PORT) < 0)
	{
		LOGERR ("Cannot bind to port %u", MDNS_PORT);

		goto InitError;
	}

	if (m_pSocket->Connect (mDNSIP, MDNS_PORT) < 0)
	{
		LOGERR ("Cannot connect to mDNS host group");

		goto InitError;
	}

	if (m_pSocket->SetOptionAddMembership (mDNSIP) < 0)
	{
		LOGERR ("Cannot join mDNS host group");

		goto InitError;
	}
	CScheduler::Get ()->MsSleep (50);	// TODO: ensure host group has been joined

	// check if our hostname is already in use
	while (!m_bRunning)
	{
		boolean bNameInUse = FALSE;
		const unsigned Tries = 3;
		for (unsigned nTry = 0; nTry < Tries; nTry++)
		{
			if (!SendMessage (FALSE))
			{
				LOGWARN ("Send query failed");
			}

			CScheduler::Get ()->MsSleep (250);

			// fetch all received messages
			int nResult;
			CIPAddress IPAddress;
			while ((nResult = ReceiveMessage (&IPAddress, FALSE)) != 0)
			{
				// was this a Response from an other host?
				if (   IPAddress.IsSet ()
				    && IPAddress != *m_pNet->GetConfig ()->GetIPAddress ())
				{
					// CString IPString;
					// IPAddress.Format (&IPString);
					// LOGDBG ("Hostname in use by %s", IPString.c_str ());

					bNameInUse = TRUE;
					nTry = Tries;
				}

				CScheduler::Get ()->Yield ();
			}
		}

		if (bNameInUse)
		{
			// set next hostname suffix
			m_Hostname.Format ("%s-%u", m_pNet->GetHostname (), ++m_nSuffix);

			m_HostnameWithDomain = m_Hostname;
			m_HostnameWithDomain.Append ("." MDNS_DOMAIN);
		}
		else
		{
			m_bRunning = TRUE;
		}
	}

	LOGNOTE ("Hostname is %s", m_Hostname.c_str ());

	while (1)
	{
		CIPAddress IPAddress;
		if (   ReceiveMessage (&IPAddress, TRUE) == 1
		    && !IPAddress.IsSet ())
		{
			if (!SendMessage (TRUE))
			{
				LOGWARN ("Send response failed");
			}
		}
	}

InitError:
	delete m_pSocket;
	m_pSocket = nullptr;

	while (1)
	{
		CScheduler::Get ()->Sleep (10);
	}
}

CmDNSDaemon *CmDNSDaemon::Get (void)
{
	if (!s_pThis)
	{
		CmDNSDaemon *pThis = new CmDNSDaemon (CNetSubSystem::Get ());

		while (!pThis->IsRunning ())
		{
			CScheduler::Get ()->MsSleep (50);
		}

		assert (pThis == s_pThis);
	}

	return s_pThis;
}

boolean CmDNSDaemon::SendMessage (boolean bResponse)
{
	u8 Buffer[DNS_MAX_MESSAGE_SIZE];
	memset (Buffer, 0, sizeof Buffer);
	TDNSHeader *pDNSHeader = reinterpret_cast<TDNSHeader *> (Buffer);

	if (!bResponse)
	{
		pDNSHeader->nID = le2be16 (m_usXID++);
		pDNSHeader->nFlags = BE (DNS_FLAGS_OPCODE_QUERY);
		pDNSHeader->nQDCount = BE (1);
	}
	else
	{
		// pDNSHeader->nID = BE (0);
		pDNSHeader->nFlags = BE ( DNS_FLAGS_QR
					| DNS_FLAGS_OPCODE_QUERY
					| DNS_FLAGS_AA
					| DNS_RCODE_SUCCESS);
		pDNSHeader->nANCount = BE (1);
	}

	u8 *pMsg = Buffer + sizeof (TDNSHeader);

	char Hostname[MAX_HOSTNAME_SIZE];
	strncpy (Hostname, m_HostnameWithDomain, MAX_HOSTNAME_SIZE-1);
	Hostname[MAX_HOSTNAME_SIZE-1] = '\0';

	char *pSavePtr;
	size_t nLength;
	char *pLabel = strtok_r (Hostname, ".", &pSavePtr);
	while (pLabel != 0)
	{
		nLength = strlen (pLabel);
		if ((int) (nLength+1+1) >= DNS_MAX_MESSAGE_SIZE-(pMsg-Buffer))
		{
			return FALSE;
		}

		*pMsg++ = static_cast<u8> (nLength);

		strcpy (reinterpret_cast<char *> (pMsg), pLabel);
		pMsg += nLength;

		pLabel = strtok_r (nullptr, ".", &pSavePtr);
	}

	*pMsg++ = '\0';

	if (!bResponse)
	{
		TDNSQueryTrailer *pTrailer = reinterpret_cast<TDNSQueryTrailer *> (pMsg);
		if ((int) (sizeof *pTrailer) > DNS_MAX_MESSAGE_SIZE-(pMsg-Buffer))
		{
			return FALSE;
		}

		pTrailer->nQType  = BE (DNS_QTYPE_A);
		pTrailer->nQClass = BE (DNS_QCLASS_IN);

		pMsg += sizeof *pTrailer;
	}
	else
	{
		TDNSResourceRecordTrailerAIN *pTrailer =
			reinterpret_cast<TDNSResourceRecordTrailerAIN *> (pMsg);
		if ((int) (sizeof *pTrailer) > DNS_MAX_MESSAGE_SIZE-(pMsg-Buffer))
		{
			return FALSE;
		}

		pTrailer->nType = BE (DNS_QTYPE_A);
		pTrailer->nClass = BE (DNS_QCLASS_IN | DNS_QCLASS_CACHE_FLUSH);
		pTrailer->nTTL = le2be32 (DNS_TTL_DEFAULT);
		pTrailer->nRDLength = BE (DNS_RDLENGTH_AIN);
		assert (m_pNet);
		m_pNet->GetConfig ()->GetIPAddress ()->CopyTo (pTrailer->RData);

		pMsg += sizeof *pTrailer;
	}

	int nSize = pMsg - Buffer;
	assert (nSize <= DNS_MAX_MESSAGE_SIZE);

	assert (m_pSocket);
	return m_pSocket->Send (Buffer, nSize, MSG_DONTWAIT);
}

int CmDNSDaemon::ReceiveMessage (CIPAddress *pIPAddress, boolean bWait)
{
	u8 Buffer[FRAME_BUFFER_SIZE];
	CIPAddress ForeignIP;
	u16 usForeignPort;
	int nResult = m_pSocket->ReceiveFrom (Buffer, sizeof Buffer, bWait ? 0 : MSG_DONTWAIT,
					      &ForeignIP, &usForeignPort);
	if (nResult == 0)
	{
		return 0;
	}
	else if (nResult < 0)
	{
		LOGWARN ("Receive failed");

		return 0;
	}

#ifndef NDEBUG
	// debug_hexdump (Buffer, nResult, From);
#endif

	TDNSHeader *pDNSHeader = (TDNSHeader *) Buffer;
	if (pDNSHeader->nFlags & BE (DNS_FLAGS_QR))
	{
		return ParseResponse (Buffer, nResult, pIPAddress);
	}
	else
	{
		return ParseQuery (Buffer, nResult);
	}
}

int CmDNSDaemon::ParseQuery (const u8 *pBuffer, size_t nBufLen)
{
	const TDNSHeader *pDNSHeader = reinterpret_cast<const TDNSHeader *> (pBuffer);
	if (nBufLen < sizeof *pDNSHeader)
	{
		return -1;
	}

	if (  (pDNSHeader->nFlags & BE (  DNS_FLAGS_QR
					| DNS_FLAGS_OPCODE
					| DNS_FLAGS_TC))
	       != BE (DNS_FLAGS_OPCODE_QUERY)
	    || pDNSHeader->nQDCount == BE (0)
	    || pDNSHeader->nANCount != BE (0))
	{
		return -1;
	}

	const u8 *pMsg = pBuffer + sizeof (TDNSHeader);

	CString Name;
	u8 uchLen;
	boolean bFirst = TRUE;
	while (   nBufLen
	       && (uchLen = *pMsg++) != 0)
	{
		nBufLen--;

		if ((uchLen & 0xC0) == 0xC0)	// compression not allowed here
		{
			return -1;
		}

		if (nBufLen < uchLen)
		{
			return -1;
		}

		char Buffer[uchLen+1];
		memcpy (Buffer, pMsg, uchLen);
		Buffer[uchLen] = '\0';

		pMsg += uchLen;
		nBufLen -= uchLen;

		if (!bFirst)
		{
			Name.Append (".");
		}

		Name.Append (Buffer);

		bFirst = FALSE;
	}

	// LOGDBG ("Query %s", Name.c_str ());

	if (Name.Compare (m_HostnameWithDomain) != 0)
	{
		return -1;
	}

	const TDNSQueryTrailer *pTrailer = reinterpret_cast<const TDNSQueryTrailer *> (pMsg);
	if (   nBufLen < sizeof *pTrailer
	    || (   pTrailer->nQType != BE (DNS_QTYPE_A)
	        && pTrailer->nQType != BE (MDNS_QTYPE_ANY))
	    || pTrailer->nQClass != BE (DNS_QCLASS_IN))
	{
		return -1;
	}

	return 1;
}

int CmDNSDaemon::ParseResponse (const u8 *pBuffer, size_t nBufLen, CIPAddress *pIPAddress)
{
	const TDNSHeader *pDNSHeader = reinterpret_cast<const TDNSHeader *> (pBuffer);
	if (nBufLen < sizeof *pDNSHeader)
	{
		return -1;
	}

	if (  (pDNSHeader->nFlags & BE (  DNS_FLAGS_QR
					| DNS_FLAGS_OPCODE
					| DNS_FLAGS_AA
					| DNS_FLAGS_TC
					| DNS_FLAGS_RCODE))
	       != BE (DNS_FLAGS_QR | DNS_FLAGS_OPCODE_QUERY | DNS_FLAGS_AA | DNS_RCODE_SUCCESS)
	    || pDNSHeader->nQDCount != BE (0)
	    || pDNSHeader->nANCount == BE (0))
	{
		return -1;
	}

	const u8 *pMsg = pBuffer + sizeof (TDNSHeader);
	nBufLen -= sizeof *pDNSHeader;

	TDNSResourceRecordTrailerAIN RRTrailer;

	// parse the answer section
	while (1)
	{
		CString Name;
		u8 uchLen;
		boolean bFirst = TRUE;
		while (   nBufLen
		       && (uchLen = *pMsg++) != 0)
		{
			nBufLen--;

			if ((uchLen & 0xC0) == 0xC0)	// compression not allowed here
			{
				return -1;
			}

			if (nBufLen < uchLen)
			{
				return -1;
			}

			char Buffer[uchLen+1];
			memcpy (Buffer, pMsg, uchLen);
			Buffer[uchLen] = '\0';

			pMsg += uchLen;
			nBufLen -= uchLen;

			if (!bFirst)
			{
				Name.Append (".");
			}

			Name.Append (Buffer);

			bFirst = FALSE;
		}

		if (nBufLen < sizeof RRTrailer)
		{
			return -1;
		}

		memcpy (&RRTrailer, pMsg, sizeof RRTrailer);

		if (   RRTrailer.nType			== BE (DNS_QTYPE_A)
		    && (  RRTrailer.nClass
		        & ~BE (DNS_QCLASS_CACHE_FLUSH))	== BE (DNS_QCLASS_IN)
		    && RRTrailer.nRDLength		== BE (DNS_RDLENGTH_AIN)
		    && Name.Compare (m_HostnameWithDomain) == 0)
		{
			break;
		}

		size_t nLen = DNS_RR_TRAILER_HEADER_LENGTH + be2le16 (RRTrailer.nRDLength);
		pMsg += nLen;
		nBufLen -= nLen;
	}

	assert (pIPAddress != 0);
	pIPAddress->Set (RRTrailer.RData);

	return 1;
}
