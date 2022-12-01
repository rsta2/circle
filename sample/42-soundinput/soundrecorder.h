//
// soundrecorder.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2021  R. Stange <rsta2@o2online.de>
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
#ifndef _soundrecorder_h
#define _soundrecorder_h

#include <circle/sched/task.h>
#include <circle/sched/synchronizationevent.h>
#include <circle/gpiopin.h>
#include <circle/string.h>
#include <circle/types.h>
#include <fatfs/ff.h>
#include "queue.h"

class CSoundRecorder : public CTask
{
public:
	CSoundRecorder (FATFS *pFileSystem);
	~CSoundRecorder (void);

	void Run (void);

	int Write (const void *pBuffer, size_t nCount);

private:
	CGPIOPin m_RecordButtonPin;

	unsigned m_nFileNumber;
	CString	 m_FileName;
	boolean	 m_bFileOpen;
	FIL	 m_File;

	CQueue m_Queue;
	CSynchronizationEvent m_Event;
};

#endif
