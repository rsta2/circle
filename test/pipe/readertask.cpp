//
// readertask.cpp
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
#include "readertask.h"
#include "config.h"
#include <circle/stdarg.h>
#include <circle/string.h>
#include <circle/sched/scheduler.h>

CReaderTask::CReaderTask (CPipeFile *pFile, CDevice *pStdout)
:	m_pFile (pFile),
	m_pStdout (pStdout)
{
	m_pFile->Open ();
#if NON_BLOCKING
	m_pFile->SetBlocking (FALSE);
#endif
}

CReaderTask::~CReaderTask (void)
{
	m_pFile->Close ();
}

void CReaderTask::Run (void)
{
	unsigned nCount = GetRandom (1, MAX_READS);
	Print ("R %p: Count is %u\n", this, nCount);

	for (unsigned i = 1; i <= nCount; i++)
	{
#if CHECK_STATUS
		unsigned nLoop = 0;
		CPipeFile::TStatus Status;
		do
		{
			Status = m_pFile->GetStatus ();

			CScheduler::Get ()->Yield ();

			nLoop++;
		}
		while (!Status.bReadReady);
		if (nLoop > 1)
		{
			Print ("R %p: Looped %u times\n", this, nLoop-1);
		}
#endif

		u8 Buffer[MAX_READ_BYTES];
		int nResult = m_pFile->Read (Buffer, sizeof Buffer);
		if (nResult <= 0)
		{
			Print ("R %p: Read returned %d\n", this, nResult);

			if (nResult == 0)
			{
				break;
			}
		}
		else
		{
			CString Msg;
			Msg.Format ("R %p:", this);

			for (int j = 0; j < nResult; j++)
			{
				CString Byte;
				Byte.Format (" %02X", Buffer[j]);

				Msg.Append (Byte);
			}

			Print ("%s\n", Msg.c_str ());
		}

		CScheduler::Get ()->Yield ();
	}

	Print ("R %p: Terminate\n", this);
}

void CReaderTask::Print (const char *pFormat, ...)
{
	CString String;

	va_list var;
	va_start (var, pFormat);

	String.FormatV (pFormat, var);

	va_end (var);

	m_pStdout->Write (String, String.GetLength ());
}

unsigned CReaderTask::GetRandom (unsigned nMin, unsigned nMax)
{
	return nMin + m_RNG.GetNumber () % (nMax - nMin + 1);
}
