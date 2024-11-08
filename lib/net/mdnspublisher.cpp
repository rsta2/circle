//
// mdnspublisher.cpp
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
#include <circle/net/mdnspublisher.h>
#include <circle/sched/scheduler.h>
#include <circle/net/in.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <assert.h>

#define MDNS_HOST_GROUP		{224, 0, 0, 251}
#define MDNS_PORT		5353
#define MDNS_DOMAIN		"local"

#define RR_TYPE_A		1
#define RR_TYPE_PTR		12
#define RR_TYPE_TXT		16
#define RR_TYPE_SRV		33

#define RR_CLASS_IN		1
#define RR_CACHE_FLUSH		0x8000

LOGMODULE ("mdnspub");

CmDNSPublisher::CmDNSPublisher (CNetSubSystem *pNet)
:	m_pNet (pNet),
	m_pSocket (nullptr),
	m_bRunning (FALSE),
	m_pWritePtr (nullptr),
	m_pDataLen (nullptr)
{
	SetName ("mdnspub");
}

CmDNSPublisher::~CmDNSPublisher (void)
{
	assert (!m_pSocket);

	m_bRunning = FALSE;
}

boolean CmDNSPublisher::PublishService (const char *pServiceName, const char *pServiceType,
					u16 usServicePort, const char *ppText[])
{
	if (!m_bRunning)
	{
		// Let task can run once to initialize
		CScheduler::Get ()->Yield ();

		if (!m_bRunning)
		{
			return FALSE;
		}
	}

	assert (pServiceName);
	assert (pServiceType);
	TService *pService = new TService {pServiceName, pServiceType, usServicePort, 0};
	assert (pService);

	if (ppText)
	{
		for (unsigned i = 0; i < MaxTextRecords && ppText[i]; i++)
		{
			pService->ppText[i] = new CString (ppText[i]);
			assert (pService->ppText[i]);

			pService->nTextRecords++;
		}
	}

	m_Mutex.Acquire ();

	// Insert as first element into list
	TPtrListElement *pElement = m_ServiceList.GetFirst ();
	if (pElement)
	{
		m_ServiceList.InsertBefore (pElement, pService);
	}
	else
	{
		m_ServiceList.InsertAfter (nullptr, pService);
	}

	m_Mutex.Release ();

	LOGDBG ("Publish service %s", (const char *) pService->ServiceName);

	m_Event.Set ();		// Trigger resent for everything

	return TRUE;
}

boolean CmDNSPublisher::UnpublishService (const char *pServiceName)
{
	if (!m_bRunning)
	{
		return FALSE;
	}

	assert (pServiceName);

	m_Mutex.Acquire ();

	// Find service in the list and remove it
	TService *pService = nullptr;
	TPtrListElement *pElement = m_ServiceList.GetFirst ();
	while (pElement)
	{
		pService = static_cast<TService *> (CPtrList::GetPtr (pElement));
		assert (pService);
		if (pService->ServiceName.Compare (pServiceName) == 0)
		{
			m_ServiceList.Remove (pElement);
			break;
		}

		pService = nullptr;

		pElement = m_ServiceList.GetNext (pElement);
	}

	m_Mutex.Release ();

	if (!pService)
	{
		return FALSE;
	}

	LOGDBG ("Unpublish service %s", (const char *) pService->ServiceName);

	if (!SendResponse (pService, TRUE))
	{
		LOGWARN ("Send failed");
	}

	for (unsigned i = 0; i < pService->nTextRecords; i++)
	{
		delete pService->ppText[i];
	}

	delete pService;

	return TRUE;
}

void CmDNSPublisher::Run (void)
{
	assert (m_pNet);
	assert (!m_pSocket);
	m_pSocket = new CSocket (m_pNet, IPPROTO_UDP);
	assert (m_pSocket);

	if (m_pSocket->Bind (MDNS_PORT) < 0)
	{
		LOGERR ("Cannot bind to port %u", MDNS_PORT);

		delete m_pSocket;
		m_pSocket = nullptr;

		while (1)
		{
			m_Event.Clear ();
			m_Event.Wait ();
		}
	}

	static const u8 mDNSIPAddress[] = MDNS_HOST_GROUP;
	CIPAddress mDNSIP (mDNSIPAddress);
	if (m_pSocket->Connect (mDNSIP, MDNS_PORT) < 0)
	{
		LOGERR ("Cannot connect to mDNS host group");

		delete m_pSocket;
		m_pSocket = nullptr;

		while (1)
		{
			m_Event.Clear ();
			m_Event.Wait ();
		}
	}

	m_bRunning = TRUE;

	while (1)
	{
		m_Event.Clear ();
		m_Event.WaitWithTimeout ((TTLShort - 10) * 1000000);

		for (unsigned i = 1; i <= 3; i++)
		{
			m_Mutex.Acquire ();

			TPtrListElement *pElement = m_ServiceList.GetFirst ();
			while (pElement)
			{
				TService *pService =
					static_cast<TService *> (CPtrList::GetPtr (pElement));
				assert (pService);

				if (!SendResponse (pService, FALSE))
				{
					LOGWARN ("Send failed");
				}

				pElement = m_ServiceList.GetNext (pElement);
			}

			m_Mutex.Release ();

			CScheduler::Get ()->Sleep (1);
		}
	}
}

boolean CmDNSPublisher::SendResponse (TService *pService, boolean bDelete)
{
	assert (pService);
	assert (m_pNet);

	// Collect data

	static const char Domain[] = "." MDNS_DOMAIN;

	CString ServiceType (pService->ServiceType);
	ServiceType.Append (Domain);

	CString ServiceName (pService->ServiceName);
	ServiceName.Append (".");
	ServiceName.Append (ServiceType);

	CString Hostname (m_pNet->GetHostname ());
	Hostname.Append (Domain);

	// Start writing buffer

	assert (!m_pWritePtr);
	m_pWritePtr = m_Buffer;

	// mDNS Header

	PutWord (0);		// Transaction ID
	PutWord (0x8400);	// Message is a response, Server is an authority for the domain
	PutWord (0);		// Questions
	PutWord (5); 		// Answer RRs
	PutWord (0);		// Authority RRs
	PutWord (0);		// Additional RRs

	// Answer RRs

	// PTR
	PutDNSName ("_services._dns-sd._udp.local");
	PutWord (RR_TYPE_PTR);
	PutWord (RR_CLASS_IN);
	PutDWord (bDelete ? TTLDelete : TTLLong);
	ReserveDataLength ();
	u8 *pServiceTypePtr = m_pWritePtr;
	PutDNSName (ServiceType);
	SetDataLength ();

	// PTR
	PutCompressedString (pServiceTypePtr);
	PutWord (RR_TYPE_PTR);
	PutWord (RR_CLASS_IN);
	PutDWord (bDelete ? TTLDelete : TTLLong);
	ReserveDataLength ();
	u8 *pServiceNamePtr = m_pWritePtr;
	PutDNSName (ServiceName);
	SetDataLength ();

	// SRV
	PutCompressedString (pServiceNamePtr);
	PutWord (RR_TYPE_SRV);
	PutWord (RR_CLASS_IN | RR_CACHE_FLUSH);
	PutDWord (bDelete ? TTLDelete : TTLShort);
	ReserveDataLength ();
	PutWord (0);		// Priority
	PutWord (0);		// Weight
	PutWord (pService->usServicePort);
	u8 *pHostnamePtr = m_pWritePtr;
	PutDNSName (Hostname);
	SetDataLength ();

	// A
	PutCompressedString (pHostnamePtr);
	PutWord (RR_TYPE_A);
	PutWord (RR_CLASS_IN | RR_CACHE_FLUSH);
	PutDWord (TTLShort);
	ReserveDataLength ();
	PutIPAddress (*m_pNet->GetConfig ()->GetIPAddress ());
	SetDataLength ();

	// TXT
	PutCompressedString (pServiceNamePtr);
	PutWord (RR_TYPE_TXT);
	PutWord (RR_CLASS_IN | RR_CACHE_FLUSH);
	PutDWord (bDelete ? TTLDelete : TTLLong);
	ReserveDataLength ();
	for (int i = pService->nTextRecords-1; i >= 0; i--)	// In reverse order
	{
		assert (pService->ppText[i]);
		PutString (*pService->ppText[i]);
	}
	SetDataLength ();

	unsigned nMsgSize = m_pWritePtr - m_Buffer;

	m_pWritePtr = nullptr;

	if (nMsgSize >= MaxMessageSize)
	{
		return FALSE;
	}

	assert (m_pSocket);
	return m_pSocket->Send (m_Buffer, nMsgSize, MSG_DONTWAIT) == (int) nMsgSize;
}

void CmDNSPublisher::PutByte (u8 uchValue)
{
	assert (m_pWritePtr);
	if ((unsigned) (m_pWritePtr - m_Buffer) < MaxMessageSize)
	{
		*m_pWritePtr++ = uchValue;
	}
}

void CmDNSPublisher::PutWord (u16 usValue)
{
	PutByte (usValue >> 8);
	PutByte (usValue & 0xFF);
}

void CmDNSPublisher::PutDWord (u32 nValue)
{
	PutWord (nValue >> 16);
	PutWord (nValue & 0xFFFF);
}

void CmDNSPublisher::PutString (const char *pValue)
{
	assert (pValue);
	size_t nLen = strlen (pValue);
	assert (nLen <= 255);

	PutByte (nLen);

	while (*pValue)
	{
		PutByte (static_cast<u8> (*pValue++));
	}
}

void CmDNSPublisher::PutCompressedString (const u8 *pWritePtr)
{
	assert (m_pWritePtr);
	assert (pWritePtr < m_pWritePtr);
	unsigned nOffset = pWritePtr - m_Buffer;
	assert (nOffset < MaxMessageSize);

	nOffset |= 0xC000;

	PutWord (static_cast<u16> (nOffset));
}

void CmDNSPublisher::PutDNSName (const char *pValue)
{
	char Buffer[256];
	assert (pValue);
	strncpy (Buffer, pValue, sizeof Buffer);
	Buffer[sizeof Buffer-1] = '\0';

	char *pSavePtr = nullptr;
	char *pToken = strtok_r (Buffer, ".", &pSavePtr);
	while (pToken)
	{
		PutString (pToken);

		pToken = strtok_r (nullptr, ".", &pSavePtr);
	}

	PutByte (0);
}

void CmDNSPublisher::PutIPAddress (const CIPAddress &rValue)
{
	u8 Buffer[IP_ADDRESS_SIZE];
	rValue.CopyTo (Buffer);

	for (unsigned i = 0; i < IP_ADDRESS_SIZE; i++)
	{
		PutByte (Buffer[i]);
	}
}

void CmDNSPublisher::ReserveDataLength (void)
{
	assert (!m_pDataLen);
	m_pDataLen = m_pWritePtr;
	assert (m_pDataLen);

	PutWord (0);
}

void CmDNSPublisher::SetDataLength (void)
{
	assert (m_pDataLen);
	assert (m_pWritePtr);
	assert (m_pWritePtr > m_pDataLen);
	*reinterpret_cast<u16 *> (m_pDataLen) = le2be16 (m_pWritePtr - m_pDataLen - sizeof (u16));

	m_pDataLen = nullptr;
}
