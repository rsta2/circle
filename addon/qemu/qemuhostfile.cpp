//
// qemuhostfile.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2020  R. Stange <rsta2@o2online.de>
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
#include <qemu/qemuhostfile.h>
#include <circle/util.h>

CQEMUHostFile::CQEMUHostFile (const char *pFileName, boolean bWrite)
{
	m_nHandle = CallSemihosting (SEMIHOSTING_SYS_OPEN, (uintptr) pFileName,
				     bWrite ? SEMIHOSTING_OPEN_WRITE : SEMIHOSTING_OPEN_READ,
				     strlen (pFileName));
}

CQEMUHostFile::~CQEMUHostFile (void)
{
	if (m_nHandle != SEMIHOSTING_NO_HANDLE)
	{
		CallSemihosting (SEMIHOSTING_SYS_CLOSE, m_nHandle);

		m_nHandle = SEMIHOSTING_NO_HANDLE;
	}
}

boolean CQEMUHostFile::IsOpen (void) const
{
	return m_nHandle != SEMIHOSTING_NO_HANDLE;
}

int CQEMUHostFile::Read (void *pBuffer, size_t nCount)
{
	int nResult = -1;

	TSemihostingValue nBytesLeft;
	if (   m_nHandle != SEMIHOSTING_NO_HANDLE
	    && (nBytesLeft = CallSemihosting (SEMIHOSTING_SYS_READ, m_nHandle,
					      (uintptr) pBuffer, nCount)) <= nCount)
	{
		nResult = (int) (nCount - nBytesLeft);
	}

	return nResult;
}

int CQEMUHostFile::Write (const void *pBuffer, size_t nCount)
{
	int nResult = -1;

	if (   m_nHandle != SEMIHOSTING_NO_HANDLE
	    && CallSemihosting (SEMIHOSTING_SYS_WRITE, m_nHandle, (uintptr) pBuffer, nCount) == 0)
	{
		nResult = (int) nCount;
	}

	return nResult;
}
