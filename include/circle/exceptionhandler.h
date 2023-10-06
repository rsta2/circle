//
// exceptionhandler.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2023  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_exceptionhandler_h
#define _circle_exceptionhandler_h

#include <circle/exception.h>
#include <circle/exceptionstub.h>

class CExceptionHandler
{
public:
	CExceptionHandler (void);
	virtual ~CExceptionHandler (void);

#if AARCH == 32
	virtual void Throw (unsigned nException);
#endif

	virtual void Throw (unsigned nException, TAbortFrame *pFrame);
	
	static CExceptionHandler *Get (void);

private:
	static const char *s_pExceptionName[];
	
	static CExceptionHandler *s_pThis;
};

#endif
