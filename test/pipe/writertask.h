//
// writertask.h
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
#ifndef _writertask_h
#define _writertask_h

#include <circle/sched/task.h>
#include <circle/sched/pipe.h>
#include <circle/device.h>
#include <circle/bcmrandom.h>

class CWriterTask : public CTask
{
public:
	CWriterTask (CPipeFile *pFile, CDevice *pStdout);
	~CWriterTask (void);

	void Run (void);

private:
	void Print (const char *pFormat, ...);

	unsigned GetRandom (unsigned nMin, unsigned nMax);

private:
	CPipeFile *m_pFile;
	CDevice *m_pStdout;

	CBcmRandomNumberGenerator m_RNG;
};

#endif
