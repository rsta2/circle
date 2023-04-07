//
// exceptionhandler.cpp
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
#include <circle/exceptionhandler.h>
#include <circle/synchronize.h>
#include <circle/logger.h>
#include <circle/debug.h>
#include <circle/multicore.h>
#include <circle/sysconfig.h>
#include <circle/string.h>
#include <circle/macros.h>
#include <assert.h>

#ifndef ARM_ALLOW_MULTI_CORE
static const char FromExcept[] = "except";
#endif

// order must match exception identifiers in circle/exception.h
const char *CExceptionHandler::s_pExceptionName[] =
{
	"Division by zero",
	"Undefined instruction",
	"Prefetch abort",
	"Data abort",
	"Unknown"
};

CExceptionHandler *CExceptionHandler::s_pThis = 0;

CExceptionHandler::CExceptionHandler (void)
{
	assert (s_pThis == 0);
	s_pThis = this;

#ifndef USE_RPI_STUB_AT
	TExceptionTable * volatile pTable = (TExceptionTable * volatile) ARM_EXCEPTION_TABLE_BASE;

	pTable->UndefinedInstruction = ARM_OPCODE_BRANCH (ARM_DISTANCE (
					pTable->UndefinedInstruction, UndefinedInstructionStub));

	pTable->PrefetchAbort = ARM_OPCODE_BRANCH (ARM_DISTANCE (
					pTable->PrefetchAbort, PrefetchAbortStub));

	pTable->DataAbort = ARM_OPCODE_BRANCH (ARM_DISTANCE (pTable->DataAbort, DataAbortStub));

	SyncDataAndInstructionCache ();
#endif
}

CExceptionHandler::~CExceptionHandler (void)
{
	s_pThis = 0;
}

void CExceptionHandler::Throw (unsigned nException)
{
#ifdef ARM_ALLOW_MULTI_CORE
	CString FromExcept;
	FromExcept.Format ("core%u", CMultiCoreSupport::ThisCore ());
#endif

	CLogger::Get()->Write (FromExcept, LogPanic, "Exception: %s", s_pExceptionName[nException]);
}

void CExceptionHandler::Throw (unsigned nException, TAbortFrame *pFrame)
{
#ifdef ARM_ALLOW_MULTI_CORE
	CString FromExcept;
	FromExcept.Format ("core%u", CMultiCoreSupport::ThisCore ());
#endif

	u32 FSR = 0, FAR = 0;
	switch (nException)
	{
	case EXCEPTION_PREFETCH_ABORT:
		asm volatile ("mrc p15, 0, %0, c5, c0,  1" : "=r" (FSR));
		asm volatile ("mrc p15, 0, %0, c6, c0,  2" : "=r" (FAR));
		break;

	case EXCEPTION_DATA_ABORT:
		asm volatile ("mrc p15, 0, %0, c5, c0,  0" : "=r" (FSR));
		asm volatile ("mrc p15, 0, %0, c6, c0,  0" : "=r" (FAR));
		break;

	default:
		break;
	}

	assert (pFrame != 0);
	u32 lr = pFrame->lr;
	u32 sp = pFrame->sp;

	switch (pFrame->spsr & 0x1F)
	{
	case 0x11:			// FIQ mode?
		lr = pFrame->lr_fiq;
		sp = pFrame->sp_fiq;
		break;

	case 0x12:			// IRQ mode?
		lr = pFrame->lr_irq;
		sp = pFrame->sp_irq;
		break;
	}

#ifndef NDEBUG
	debug_stacktrace ((u32 *) sp, FromExcept);
#endif

	CLogger::Get()->Write (FromExcept, LogPanic,
		"%s (PC 0x%X, FSR 0x%X, FAR 0x%X, SP 0x%X, LR 0x%X, PSR 0x%X)",
		s_pExceptionName[nException],
		pFrame->pc, FSR, FAR,
		sp, lr, pFrame->spsr);
}

CExceptionHandler *CExceptionHandler::Get (void)
{
	assert (s_pThis != 0);
	return s_pThis;
}

void ExceptionHandler (u32 nException, TAbortFrame *pFrame)
{
	PeripheralExit ();	// exit from interrupted peripheral

	// if an exception occurs on FIQ_LEVEL, the system would otherwise hang in the next spin lock
	CInterruptSystem::DisableFIQ ();
	EnableFIQs ();

	CExceptionHandler::Get ()->Throw (nException, pFrame);
}

#if STDLIB_SUPPORT == 1 || STDLIB_SUPPORT == 2

extern "C" int raise (int nSignal) WEAK;

int raise (int nSignal)
{
	CExceptionHandler::Get ()->Throw (EXCEPTION_UNKNOWN);

	return 0;
}

#endif
