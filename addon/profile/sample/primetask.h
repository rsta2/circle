//
// primetask.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015  R. Stange <rsta2@o2online.de>
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
#ifndef _primetask_h
#define _primetask_h

#include <circle/sched/task.h>
#include <circle/screen.h>
#include <circle/types.h>

#define PRIME_MAX		100000000		// upper bound of calculation
#define PRIME_MAX_SQRT		10000			// square root of PRIME_MAX

#define PRIME_ARRAY_SIZE	((PRIME_MAX + 31) / 32)
 
class CPrimeTask : public CTask
{
public:
	CPrimeTask (CScreenDevice *pScreen);
	~CPrimeTask (void);

	void Run (void);

private:
	boolean IsPrime (unsigned nNumber);

	void NotPrime (unsigned nNumber);

private:
	CScreenDevice  *m_pScreen;
	u32		m_PrimeArray[PRIME_ARRAY_SIZE];
};

#endif
