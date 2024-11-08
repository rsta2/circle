//
// mdnspublisher.h
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
#ifndef _circle_net_mdnspublisher_h
#define _circle_net_mdnspublisher_h

#include <circle/sched/task.h>
#include <circle/sched/mutex.h>
#include <circle/sched/synchronizationevent.h>
#include <circle/net/netsubsystem.h>
#include <circle/net/socket.h>
#include <circle/net/ipaddress.h>
#include <circle/ptrlist.h>
#include <circle/string.h>
#include <circle/types.h>

class CmDNSPublisher : public CTask	/// mDNS / Bonjour client task
{
public:
	static constexpr const char *ServiceTypeAppleMIDI = "_apple-midi._udp";

public:
	/// \param pNet Pointer to the network subsystem object
	CmDNSPublisher (CNetSubSystem *pNet);

	~CmDNSPublisher (void);

	/// \brief Start publishing a service
	/// \param pServiceName Name of the service to be published
	/// \param pServiceType Type of the service to be published (e.g. ServiceTypeAppleMIDI)
	/// \param usServicePort Port number of the service to be published (in host byte order)
	/// \param ppText Descriptions of the service (terminated with a nullptr, or nullptr itself)
	/// \return Operation successful?
	boolean PublishService (const char *pServiceName,
				const char *pServiceType,
				u16	    usServicePort,
				const char *ppText[] = nullptr);

	/// \brief Stop publishing a service
	/// \param pServiceName Name of the service to be unpublished (same as when published)
	/// \return Operation successful?
	boolean UnpublishService (const char *pServiceName);

	void Run (void) override;

private:
	static const unsigned MaxTextRecords = 10;
	static const unsigned MaxMessageSize = 1400;	// safe UDP payload in an Ethernet frame

	static const unsigned TTLShort = 120;		// seconds
	static const unsigned TTLLong = 4500;
	static const unsigned TTLDelete = 0;

	struct TService
	{
		CString	  ServiceName;
		CString	  ServiceType;
		u16	  usServicePort;
		unsigned  nTextRecords;
		CString	 *ppText[MaxTextRecords];
	};

	boolean SendResponse (TService *pService, boolean bDelete);

	// Helpers for writing to buffer
	void PutByte (u8 uchValue);
	void PutWord (u16 usValue);
	void PutDWord (u32 nValue);
	void PutString (const char *pValue);
	void PutCompressedString (const u8 *pWritePtr);
	void PutDNSName (const char *pValue);
	void PutIPAddress (const CIPAddress &rValue);

	void ReserveDataLength (void);
	void SetDataLength (void);

private:
	CNetSubSystem *m_pNet;

	CPtrList m_ServiceList;
	CMutex m_Mutex;

	CSocket *m_pSocket;

	boolean m_bRunning;
	CSynchronizationEvent m_Event;

	u8 m_Buffer[MaxMessageSize];
	u8 *m_pWritePtr;
	u8 *m_pDataLen;
};

#endif
