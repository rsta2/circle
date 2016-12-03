//
// tftpfileserver.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016  R. Stange <rsta2@o2online.de>
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
#include <tftpfileserver/tftpfileserver.h>
#include <assert.h>

CTFTPFileServer::CTFTPFileServer (CNetSubSystem *pNetSubSystem, CFATFileSystem *pFileSystem)
:	CTFTPDaemon (pNetSubSystem),
	m_pFileSystem (pFileSystem),
	m_hFile (0)
{
}

CTFTPFileServer::~CTFTPFileServer (void)
{
	if (m_hFile != 0)
	{
		FileClose ();
	}

	m_pFileSystem = 0;
}

boolean CTFTPFileServer::FileOpen (const char *pFileName)
{
	assert (pFileName != 0);
	assert (m_pFileSystem != 0);
	assert (m_hFile == 0);
	m_hFile = m_pFileSystem->FileOpen (pFileName);

	return m_hFile != 0 ? TRUE : FALSE;
}

boolean CTFTPFileServer::FileCreate (const char *pFileName)
{
	assert (pFileName != 0);
	assert (m_pFileSystem != 0);
	assert (m_hFile == 0);
	m_hFile = m_pFileSystem->FileCreate (pFileName);

	return m_hFile != 0 ? TRUE : FALSE;
}

boolean CTFTPFileServer::FileClose (void)
{
	assert (m_pFileSystem != 0);
	assert (m_hFile != 0);
	boolean bOK = m_pFileSystem->FileClose (m_hFile) != 0 ? TRUE : FALSE;

	m_hFile = 0;

	return bOK;
}

int CTFTPFileServer::FileRead (void *pBuffer, unsigned nCount)
{
	assert (m_pFileSystem != 0);
	assert (m_hFile != 0);
	assert (pBuffer != 0);
	assert (nCount > 0);
	return (int) m_pFileSystem->FileRead (m_hFile, pBuffer, nCount);
}

int CTFTPFileServer::FileWrite (const void *pBuffer, unsigned nCount)
{
	assert (m_pFileSystem != 0);
	assert (m_hFile != 0);
	assert (pBuffer != 0);
	assert (nCount > 0);
	return (int) m_pFileSystem->FileWrite (m_hFile, pBuffer, nCount);
}
