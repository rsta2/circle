//
// tftpfatfsfileserver.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2021  R. Stange <rsta2@o2online.de>
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
#ifndef _tftpfileserver_tftpfatfsfileserver_h
#define _tftpfileserver_tftpfatfsfileserver_h

#include <circle/net/tftpdaemon.h>
#include <circle/net/netsubsystem.h>
#include <fatfs/ff.h>
#include <circle/string.h>
#include <circle/types.h>

class CTFTPFatFsFileServer : public CTFTPDaemon
{
public:
	CTFTPFatFsFileServer (CNetSubSystem *pNetSubSystem, FATFS *pFileSystem,
			      const char *pPath = "SD:/");	// must have trailing '/'
	~CTFTPFatFsFileServer (void);

	boolean FileOpen (const char *pFileName);
	boolean FileCreate (const char *pFileName);
	boolean FileClose (void);
	int FileRead (void *pBuffer, unsigned nCount);
	int FileWrite (const void *pBuffer, unsigned nCount);

private:
	FATFS *m_pFileSystem;
	CString m_Path;

	CString m_Filename;
	FIL m_File;
	boolean m_bFileOpen;
};

#endif
