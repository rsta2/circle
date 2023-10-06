//
// tftpdaemon.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2023  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_net_tftpdaemon_h
#define _circle_net_tftpdaemon_h

#include <circle/sched/task.h>
#include <circle/net/netsubsystem.h>
#include <circle/net/socket.h>
#include <circle/net/ipaddress.h>
#include <circle/types.h>

class CTFTPDaemon : public CTask
{
public:
	enum TStatus
	{
		StatusIdle,
		StatusReadInProgress,
		StatusWriteInProgress,
		StatusReadCompleted,
		StatusWriteCompleted,
		StatusReadAborted,
		StatusWriteAborted,
		StatusUnknown
	};

	static const size_t MaxFilenameLen = 128;

public:
	CTFTPDaemon (CNetSubSystem *pNetSubSystem);
	~CTFTPDaemon (void);

	void Run (void);

	virtual boolean FileOpen (const char *pFileName) = 0;
	virtual boolean FileCreate (const char *pFileName) = 0;
	virtual boolean FileClose (void) = 0;
	virtual int FileRead (void *pBuffer, unsigned nCount) = 0;
	virtual int FileWrite (const void *pBuffer, unsigned nCount) = 0;

	virtual void UpdateStatus (TStatus Status, const char *pFileName) {}

private:
	boolean DoRead (const char *pFileName);
	boolean DoWrite (const char *pFileName);

	// use m_pRequestSocket, if pSendTo/nPort are given; m_pTransferSocket otherwise
	void SendError (u16 usErrorCode, const char *pErrorMessage,
			CIPAddress *pSendTo = 0, u16 usPort = 0);

private:
	CNetSubSystem *m_pNetSubSystem;

	CSocket *m_pRequestSocket;
	CSocket *m_pTransferSocket;

	char m_Filename[MaxFilenameLen+1];
};

#endif
