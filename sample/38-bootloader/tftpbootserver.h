//
// tftpbootserver.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2019  R. Stange <rsta2@o2online.de>
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
#ifndef _tftpbootserver_h
#define _tftpbootserver_h

#include <circle/net/tftpdaemon.h>
#include <circle/net/netsubsystem.h>
#include <circle/types.h>

class CTFTPBootServer : public CTFTPDaemon
{
public:
	CTFTPBootServer (CNetSubSystem *pNetSubSystem, size_t nMaxKernelSize);
	~CTFTPBootServer (void);

	boolean FileOpen (const char *pFileName);
	boolean FileCreate (const char *pFileName);
	boolean FileClose (void);
	int FileRead (void *pBuffer, unsigned nCount);
	int FileWrite (const void *pBuffer, unsigned nCount);

private:
	size_t m_nMaxKernelSize;

	boolean m_bFileOpen;
	u8 *m_pKernelBuffer;
	unsigned m_nCurrentOffset;
};

#endif
