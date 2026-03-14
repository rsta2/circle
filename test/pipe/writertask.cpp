//
// writertask.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2026  R. Stange <rsta2@gmx.net>
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
#include "writertask.h"
#include "config.h"
#include <circle/sched/scheduler.h>
#include <circle/stdarg.h>

CWriterTask::CWriterTask (CPipeFile *pFile, CDevice *pStdout)
:	m_pFile (pFile),
	m_pStdout (pStdout)
{
	m_pFile->Open ();
#if NON_BLOCKING
	m_pFile->SetBlocking (FALSE);
#endif
}

CWriterTask::~CWriterTask (void)
{
	m_pFile->Close ();
}

void CWriterTask::Run (void)
{
	unsigned nCount = GetRandom (1, MAX_WRITES);
	Print ("W %p: Count is %u\n", this, nCount);

	for (unsigned i = 1; i <= nCount; i++)
	{
		int nBytes = GetRandom (1, MAX_WRITE_BYTES);
		u8 Buffer[nBytes];
		CString Msg;
		for (int j = 0; j < nBytes; j++)
		{
			u8 uchByte = GetRandom (0, 255);
			Buffer[j] = uchByte;

			CString Byte;
			Byte.Format (" %02X", (unsigned) uchByte);
			Msg.Append (Byte);
		}

		Print ("W %p:%s\n", this, Msg.c_str ());

#if CHECK_STATUS
		unsigned nLoop = 0;
		CPipeFile::TStatus Status;
		do
		{
			Status = m_pFile->GetStatus ();

			CScheduler::Get ()->Yield ();

			nLoop++;
		}
		while (!Status.bWriteReady);
		if (nLoop > 1)
		{
			Print ("W %p: Looped %u times\n", this, nLoop-1);
		}
#endif

		int nResult;
#if NON_BLOCKING
		nResult = m_pFile->Write (Buffer, nBytes);
#else
		while ((nResult = m_pFile->Write (Buffer, nBytes)) == -CPipeFile::WouldBlock)
		{
			Print ("W %p: Write returned %d\n", this, nResult);

			CScheduler::Get ()->Yield ();
		}
#endif
		if (nResult != nBytes)
		{
			Print ("W %p: Write returned %d\n", this, nResult);

			if (nResult == -CPipeFile::NoReader)
			{
				break;
			}
		}

#if MAX_SLEEP_MS
		CScheduler::Get ()->MsSleep (GetRandom (0, MAX_SLEEP_MS));
#else
		CScheduler::Get ()->Yield ();
#endif
	}

	Print ("W %p: Terminate\n", this);
}

void CWriterTask::Print (const char *pFormat, ...)
{
	CString String;

	va_list var;
	va_start (var, pFormat);

	String.FormatV (pFormat, var);

	va_end (var);

	m_pStdout->Write (String, String.GetLength ());
}

unsigned CWriterTask::GetRandom (unsigned nMin, unsigned nMax)
{
	return nMin + m_RNG.GetNumber () % (nMax - nMin + 1);
}
