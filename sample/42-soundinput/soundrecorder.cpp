//
// soundrecorder.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2021-2022  R. Stange <rsta2@o2online.de>
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
#include "soundrecorder.h"
#include "config.h"
#include "wavefile.h"
#include <circle/sched/scheduler.h>
#include <circle/logger.h>
#include <assert.h>

static const unsigned BitsPerSample = (WRITE_FORMAT + 1) * 8;

// queue can hold data for 5 seconds of maximum frame size
static const unsigned QueueSize = 5 * SAMPLE_RATE * WRITE_CHANNELS * sizeof (u32);

static const unsigned MaxFileNumber = 20;

static const unsigned FileChunkSize = 8192;

LOGMODULE ("recorder");

CSoundRecorder::CSoundRecorder (FATFS *pFileSystem)
:	m_RecordButtonPin (RECORD_BUTTON, GPIOModeInputPullUp),
	m_nFileNumber (0),
	m_bFileOpen (FALSE),
	m_Queue (QueueSize)
{
	SetName ("recorder");
}

CSoundRecorder::~CSoundRecorder (void)
{
	assert (0);
}

void CSoundRecorder::Run (void)
{
	while (1)
	{
		// wait for record button to be pressed
		while (m_RecordButtonPin.Read () == HIGH)
		{
			CScheduler::Get ()->MsSleep (5);
		}

		CScheduler::Get ()->MsSleep (50);		// debounce button

		while (m_RecordButtonPin.Read () == LOW)
		{
			CScheduler::Get ()->MsSleep (5);
		}

		// create file
		for (m_nFileNumber++; m_nFileNumber <= MaxFileNumber; m_nFileNumber++)
		{
			m_FileName.Format (DRIVE FILEPATTERN, m_nFileNumber);

			if (f_open (&m_File, (const char *) m_FileName,
				    FA_WRITE | FA_CREATE_NEW) == FR_OK)
			{
				LOGDBG ("File created: %s", (const char *) m_FileName);

				break;
			}
		}

		if (m_nFileNumber > MaxFileNumber)
		{
			LOGERR ("Too many files");

			goto Error;
		}

		m_bFileOpen = TRUE;

		// reserve room for WAVE header
		if (f_lseek (&m_File, sizeof (TWAVEFileHeader)) != FR_OK)
		{
			LOGERR ("Seek failed");

			goto Error;
		}

		// run queue, as long record button is not pressed
		m_Queue.Flush ();
		m_Event.Clear ();

		unsigned nDataChunkSize = 0;
		while (m_RecordButtonPin.Read () == HIGH)
		{
			m_Event.Wait ();

			unsigned nAvailable = m_Queue.GetBytesAvailable ();
			if (nAvailable >= FileChunkSize)
			{
				u8 Buffer[FileChunkSize];
				m_Queue.Read (Buffer, sizeof Buffer);

				unsigned nBytesWritten;
				if (   f_write (&m_File, Buffer, FileChunkSize,
						&nBytesWritten) != FR_OK
				    || nBytesWritten != FileChunkSize)
				{
					LOGERR ("Write failed");

					goto Error;
				}

				nDataChunkSize += nBytesWritten;
			}
			else
			{
				m_Event.Clear ();
			}
		}

		// prepare and write WAVE header
		TWAVEFileHeader Header = WAVE_FILE_HEADER (WRITE_CHANNELS, BitsPerSample,
							   SAMPLE_RATE, nDataChunkSize);

		unsigned nBytesWritten;
		if (   f_rewind (&m_File) != FR_OK
		    || f_write (&m_File, &Header, sizeof Header,
				&nBytesWritten) != FR_OK
		    || nBytesWritten != sizeof Header)
		{
			LOGERR ("Write failed");

			goto Error;
		}

		m_bFileOpen = FALSE;

		f_close (&m_File);

		LOGDBG ("File closed");

		CScheduler::Get ()->MsSleep (50);		// debounce button

		while (m_RecordButtonPin.Read () == LOW)
		{
			CScheduler::Get ()->MsSleep (5);
		}
	}

Error:
	if (m_bFileOpen)
	{
		m_bFileOpen = FALSE;

		f_close (&m_File);
	}

	while (1)
	{
		CScheduler::Get ()->Sleep (10);
	}
}

int CSoundRecorder::Write (const void *pBuffer, size_t nCount)
{
	if (!m_bFileOpen)
	{
		return nCount;
	}

	unsigned nFree = m_Queue.GetFreeSpace ();
	if (nFree < nCount)
	{
		nCount = nFree;
	}

	if (nCount > 0)
	{
		m_Queue.Write (pBuffer, nCount);

		if (m_Queue.GetBytesAvailable () >= FileChunkSize)
		{
			m_Event.Set ();
		}
	}

	return nCount;
}
