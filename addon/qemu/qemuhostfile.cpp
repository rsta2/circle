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

#define SYS_OPEN	0x01
	#define SYS_OPEN_MODE_READ	0
	#define SYS_OPEN_MODE_WRITE	4
	#define SYS_OPEN_NO_HANDLE	((value) -1)
#define SYS_CLOSE	0x02
#define SYS_WRITE	0x05
	#define SYS_WRITE_OK		0
#define SYS_READ	0x06

CQEMUHostFile::CQEMUHostFile (const char *pFileName, boolean bWrite)
{
	m_nHandle = CallSemihosting (SYS_OPEN, (uintptr) pFileName,
				     bWrite ? SYS_OPEN_MODE_WRITE : SYS_OPEN_MODE_READ,
				     strlen (pFileName));
}

CQEMUHostFile::~CQEMUHostFile (void)
{
	if (m_nHandle != SYS_OPEN_NO_HANDLE)
	{
		CallSemihosting (SYS_CLOSE, m_nHandle);

		m_nHandle = SYS_OPEN_NO_HANDLE;
	}
}

boolean CQEMUHostFile::IsOpen (void) const
{
	return m_nHandle != SYS_OPEN_NO_HANDLE;
}

int CQEMUHostFile::Read (void *pBuffer, size_t nCount)
{
	int nResult = -1;

	value nBytesLeft;
	if (   m_nHandle != SYS_OPEN_NO_HANDLE
	    && (nBytesLeft = CallSemihosting (SYS_READ, m_nHandle,
					      (uintptr) pBuffer, nCount)) <= nCount)
	{
		nResult = (int) (nCount - nBytesLeft);
	}

	return nResult;
}

int CQEMUHostFile::Write (const void *pBuffer, size_t nCount)
{
	int nResult = -1;

	if (   m_nHandle != SYS_OPEN_NO_HANDLE
	    && CallSemihosting (SYS_WRITE, m_nHandle, (uintptr) pBuffer, nCount) == SYS_WRITE_OK)
	{
		nResult = (int) nCount;
	}

	return nResult;
}

CQEMUHostFile::value CQEMUHostFile::CallSemihosting (value nOperation, value nParam1,
						     value nParam2, value nParam3)
{
	volatile value Data[3] = {nParam1, nParam2, nParam3};
	value nResult;

#if AARCH == 32
	asm volatile
	(
		"mov r0, %1\n"
		"mov r1, %2\n"
		"svc 0x123456\n"
		"mov %0, r0\n"

		: "=r" (nResult) : "r" (nOperation), "r" ((uintptr) &Data)
	);
#else
	asm volatile
	(
		"mov x0, %1\n"
		"mov x1, %2\n"
		"hlt #0xF000\n"
		"mov %0, x0\n"

		: "=r" (nResult) : "r" (nOperation), "r" ((uintptr) &Data)
	);
#endif

	return nResult;
}
