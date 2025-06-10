//
// mdnsdaemon.h
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
#ifndef _circle_net_mdnsdaemon_h
#define _circle_net_mdnsdaemon_h

#include <circle/sched/task.h>
#include <circle/net/netsubsystem.h>
#include <circle/net/ipaddress.h>
#include <circle/net/socket.h>
#include <circle/string.h>
#include <circle/types.h>

class CmDNSDaemon : public CTask	/// mDNS responder task
{
public:
	/// \param pNet Pointer to network subsystem object
	CmDNSDaemon (CNetSubSystem *pNet);

	~CmDNSDaemon (void);

	/// \return TRUE if the responder has been successfully been initialized.
	boolean IsRunning (void) const;

	/// \return Our own mDNS hostname without ".local" domain suffix.
	CString GetHostname (void) const;
	/// \return Our own mDNS hostname suffix number (e.g. 2 for "-2").
	/// \note The first host with this hostname prefix has the suffix number 1,\n
	///	  which is not appended to the hostname above.
	unsigned GetSuffix (void) const;

	void Run (void) override;

	/// \return Pointer to the one and only mDNS responder task object.
	/// \note The object is automatically created, if it does not exist yet.
	static CmDNSDaemon *Get (void);

private:
	// Send Response or Query message otherwise
	boolean SendMessage (boolean bResponse);

	// Returns -1 on error, 0 on none received, or 1 on Query or Response received
	// On a Response the IP address will be set
	int ReceiveMessage (CIPAddress *pIPAddress, boolean bWait);

	int ParseQuery (const u8 *pBuffer, size_t nBufLen);
	int ParseResponse (const u8 *pBuffer, size_t nBufLen, CIPAddress *pIPAddress);

private:
	CNetSubSystem *m_pNet;

	boolean m_bRunning;
	unsigned m_nSuffix;
	CString m_Hostname;
	CString m_HostnameWithDomain;

	CSocket *m_pSocket;

	u16 m_usXID;

	static CmDNSDaemon *s_pThis;
};

#endif
