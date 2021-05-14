//
// tftpfatfsfileserver.cpp
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
#include <tftpfileserver/tftpfatfsfileserver.h>
#include <assert.h>

CTFTPFatFsFileServer::CTFTPFatFsFileServer (CNetSubSystem *pNetSubSystem, FATFS *pFileSystem,
					    const char *pPath)
:	CTFTPDaemon (pNetSubSystem),
	m_pFileSystem (pFileSystem),
	m_Path (pPath),
	m_bFileOpen (FALSE)
{
	assert (m_Path.GetLength () > 0);
	assert (((const char *) m_Path)[m_Path.GetLength ()-1] == '/');
}

CTFTPFatFsFileServer::~CTFTPFatFsFileServer (void)
{
	if (m_bFileOpen)
	{
		FileClose ();
	}

	m_pFileSystem = 0;
}

boolean CTFTPFatFsFileServer::FileOpen (const char *pFileName)
{
	assert (pFileName != 0);
	assert (m_pFileSystem != 0);
	assert (!m_bFileOpen);

	m_Filename = m_Path;
	m_Filename.Append (pFileName);

	FRESULT Result = f_open (&m_File, m_Filename, FA_READ | FA_OPEN_EXISTING);
	if (Result != FR_OK)
	{
		return FALSE;
	}

	m_bFileOpen = TRUE;

	return TRUE;
}

boolean CTFTPFatFsFileServer::FileCreate (const char *pFileName)
{
	assert (pFileName != 0);
	assert (m_pFileSystem != 0);
	assert (!m_bFileOpen);

	m_Filename = m_Path;
	m_Filename.Append (pFileName);

	FRESULT Result = f_open (&m_File, m_Filename, FA_WRITE | FA_CREATE_ALWAYS);
	if (Result != FR_OK)
	{
		return FALSE;
	}

	m_bFileOpen = TRUE;

	return TRUE;
}

boolean CTFTPFatFsFileServer::FileClose (void)
{
	assert (m_pFileSystem != 0);
	assert (m_bFileOpen);

	if (f_close (&m_File) != FR_OK)
	{
		return FALSE;
	}

	m_bFileOpen = FALSE;

	return TRUE;
}

int CTFTPFatFsFileServer::FileRead (void *pBuffer, unsigned nCount)
{
	assert (m_pFileSystem != 0);
	assert (m_bFileOpen);
	assert (pBuffer != 0);
	assert (nCount > 0);

	unsigned nBytesRead;
	FRESULT Result = f_read (&m_File, pBuffer, nCount, &nBytesRead);
	if (Result != FR_OK)
	{
		return -1;
	}

	return nBytesRead;
}

int CTFTPFatFsFileServer::FileWrite (const void *pBuffer, unsigned nCount)
{
	assert (m_pFileSystem != 0);
	assert (m_bFileOpen);
	assert (pBuffer != 0);
	assert (nCount > 0);

	unsigned nBytesWritten;
	FRESULT Result = f_write (&m_File, pBuffer, nCount, &nBytesWritten);
	if (Result != FR_OK)
	{
		return -1;
	}

	return nBytesWritten;
}
