//
// exceptionhandler64.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2018  R. Stange <rsta2@o2online.de>
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
#include <circle/exceptionhandler.h>
#include <circle/logger.h>
#include <circle/interrupt.h>
#include <circle/synchronize.h>
#include <circle/debug.h>
#include <circle/sysconfig.h>
#include <circle/string.h>
#include <assert.h>

static const char FromExcept[] = "except";

// order must match exception identifiers in circle/exception.h
const char *CExceptionHandler::s_pExceptionName[] =
{
	"Unexpected exception",
	"Synchronous exception",
	"System error"
};

CExceptionHandler *CExceptionHandler::s_pThis = 0;

CExceptionHandler::CExceptionHandler (void)
{
	assert (s_pThis == 0);
	s_pThis = this;
}

CExceptionHandler::~CExceptionHandler (void)
{
	s_pThis = 0;
}

void CExceptionHandler::Throw (unsigned nException, TAbortFrame *pFrame)
{
	assert (pFrame != 0);

	u64 sp = pFrame->sp_el0;
	if ((pFrame->spsr_el1 & 0x0F) == 0x05)		// EL1h mode?
	{
		sp = pFrame->sp_el1;
	}

	u64 nEC  = (pFrame->esr_el1 >> 26) & 0x3F;
	u64 nISS = pFrame->esr_el1 & 0x1FFFFFF;

	u64 nFAR = 0;
	if (   (0x20 <= nEC && nEC <= 0x25)
	    || (0x34 <= nEC && nEC <= 0x35))
	{
		nFAR = pFrame->far_el1;
	}

#ifndef NDEBUG
	debug_stacktrace ((u64 *) sp, FromExcept);
#endif

	CLogger::Get()->Write (FromExcept, LogPanic,
		"%s (PC 0x%lX, EC 0x%lX, ISS 0x%lX, FAR 0x%lX, SP 0x%lX, LR 0x%lX, SPSR 0x%lX)",
		s_pExceptionName[nException],
		pFrame->elr_el1, nEC, nISS, nFAR, sp, pFrame->x30, pFrame->spsr_el1);
}

CExceptionHandler *CExceptionHandler::Get (void)
{
	assert (s_pThis != 0);
	return s_pThis;
}

void ExceptionHandler (u64 nException, TAbortFrame *pFrame)
{
	// if an exception occurs on FIQ_LEVEL, the system would otherwise hang in the next spin lock
	CInterruptSystem::DisableFIQ ();
	EnableFIQs ();

	CExceptionHandler::Get ()->Throw (nException, pFrame);
}
