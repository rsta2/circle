//
// multicore.cpp
//
// Some information in this file is taken from Linux and is:
//	Copyright (C) Broadcom
//	Licensed under GPL2
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2023  R. Stange <rsta2@o2online.de>
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
#include <circle/multicore.h>

#ifdef ARM_ALLOW_MULTI_CORE

#include <circle/startup.h>
#include <circle/bcm2836.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/memio.h>
#include <circle/spinlock.h>
#include <assert.h>

static const char FromMultiCore[] = "mcore";

CMultiCoreSupport *CMultiCoreSupport::s_pThis = 0;

CMultiCoreSupport::CMultiCoreSupport (CMemorySystem *pMemorySystem)
:	m_pMemorySystem (pMemorySystem)
{
	assert (s_pThis == 0);
	s_pThis = this;

#if RASPPI >= 5
	for (unsigned nCore = 1; nCore < CORES; nCore++)
	{
		m_bCoreStarted[nCore] = FALSE;
	}
#endif
}

CMultiCoreSupport::~CMultiCoreSupport (void)
{
	m_pMemorySystem = 0;

	s_pThis = 0;
}

#if RASPPI <= 4

boolean CMultiCoreSupport::Initialize (void)
{
#if RASPPI <= 3
	// Route IRQ and FIQ to core 0
	u32 nRouting = read32 (ARM_LOCAL_GPU_INT_ROUTING);
	nRouting &= ~0x0F;
	write32 (ARM_LOCAL_GPU_INT_ROUTING, nRouting);

	write32 (ARM_LOCAL_MAILBOX_INT_CONTROL0, 1);		// enable IPI on core 0
#endif

	CSpinLock::Enable ();

	CleanDataCache ();	// write out all data to be accessible by secondary cores

	for (unsigned nCore = 1; nCore < CORES; nCore++)
	{
#if AARCH == 32
		u32 nMailBoxClear = ARM_LOCAL_MAILBOX3_CLR0 + 0x10 * nCore;

		DataSyncBarrier ();
#else
		TSpinTable * volatile pSpinTable = (TSpinTable * volatile) ARM_SPIN_TABLE_BASE;
#endif

		unsigned nTimeout = 100;
#if AARCH == 32
		while (read32 (nMailBoxClear) != 0)
#else
		while (pSpinTable->SpinCore[nCore] != 0)
#endif
		{
			if (--nTimeout == 0)
			{
				CLogger::Get ()->Write (FromMultiCore, LogError, "CPU core %u does not respond", nCore);

				return FALSE;
			}

			CTimer::SimpleMsDelay (1);
		}

#if AARCH == 32
		write32 (ARM_LOCAL_MAILBOX3_SET0 + 0x10 * nCore, (u32) &_start_secondary);
#else
		pSpinTable->SpinCore[nCore] = (uintptr) &_start_secondary;
		// TODO: CleanDataCacheRange ((u64) pSpinTable, sizeof *pSpinTable);
		CleanDataCache ();
#endif

		nTimeout = 500;
		do
		{
			asm volatile ("sev");

			if (--nTimeout == 0)
			{
				CLogger::Get ()->Write (FromMultiCore, LogError, "CPU core %u did not start", nCore);

				return FALSE;
			}

			CTimer::SimpleMsDelay (1);

			DataMemBarrier ();
		}
#if AARCH == 32
		while (read32 (nMailBoxClear) != 0);
#else
		while (pSpinTable->SpinCore[nCore] != 0);
#endif
	}

	return TRUE;
}

#else	// #if RASPPI <= 4

boolean CMultiCoreSupport::Initialize (void)
{
	CSpinLock::Enable ();

	CleanDataCache ();	// write out all data to be accessible by secondary cores

	// start all secondary cores
	for (unsigned nCore = 1; nCore < CORES; nCore++)
	{
		// call PSCI function CPU_ON
		// see: ARM Power State Coordination Interface (DEN 0022D, section 5.1.4),
		//	SMC CALLING CONVENTION (DEN 0028B, section 2.7)
		s32 nReturnCode;
		asm volatile
		(
			"mov	x0, %1\n"
			"mov	x1, %2\n"
			"mov	x2, %3\n"
			"mov	x3, %4\n"
			"smc	#0\n"
			"mov	%w0, w0\n"

			: "=r" (nReturnCode)

			: "r" (0xC4000003UL),			// function code CPU_ON
			  "r" ((u64) nCore << 8),		// target core
			  "r" ((u64) &_start_secondary),	// entry point
			  "i" (0)				// context (unused)

			: "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8", "x9",
			  "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17"
		);

		if (nReturnCode != 0)
		{
			CLogger::Get ()->Write (FromMultiCore, LogError, "CPU core %u did not start (%d)",
						nCore, nReturnCode);

			return FALSE;
		}
	}

	// check, that the secondary cores responded
	for (unsigned nCore = 1; nCore < CORES; nCore++)
	{
		unsigned nTimeout = 500;
		do
		{
			if (--nTimeout == 0)
			{
				CLogger::Get ()->Write (FromMultiCore, LogError,
							"CPU core %u did not start", nCore);

				return FALSE;
			}

			CTimer::SimpleMsDelay (1);

			DataMemBarrier ();
		}
		while (!m_bCoreStarted[nCore]);
	}

	return TRUE;
}

#endif

void CMultiCoreSupport::IPIHandler (unsigned nCore, unsigned nIPI)
{
	assert (nCore < CORES);
	assert (nIPI <= IPI_MAX);

	if (nIPI == IPI_HALT_CORE)
	{
		CLogger::Get ()->Write (FromMultiCore, LogDebug, "CPU core %u will halt now", nCore);

		halt ();
	}
}

void CMultiCoreSupport::SendIPI (unsigned nCore, unsigned nIPI)
{
	assert (nCore < CORES);
	assert (nIPI <= IPI_MAX);

#if RASPPI <= 3
	write32 (ARM_LOCAL_MAILBOX0_SET0 + 0x10 * nCore, 1 << nIPI);
#else
	CInterruptSystem::SendIPI (nCore, nIPI);
#endif
}

void CMultiCoreSupport::HaltAll (void)
{
	for (unsigned nCore = 0; nCore < CORES; nCore++)
	{
		if (ThisCore () != nCore)
		{
			SendIPI (nCore, IPI_HALT_CORE);
		}
	}
	
	halt ();
}

#if RASPPI <= 3

boolean CMultiCoreSupport::LocalInterruptHandler (void)
{
	if (s_pThis == 0)
	{
		return FALSE;
	}

	unsigned nCore = ThisCore ();
	if (!(read32 (ARM_LOCAL_IRQ_PENDING0 + 4 * nCore) & 0x10))
	{
		return FALSE;
	}

	uintptr nMailBoxClear = ARM_LOCAL_MAILBOX0_CLR0 + 0x10 * nCore;
	u32 nIPIMask = read32 (nMailBoxClear);
	if (nIPIMask == 0)
	{
		return FALSE;
	}

	unsigned nIPI;
	for (nIPI = 0; !(nIPIMask & 1); nIPI++)
	{
		nIPIMask >>= 1;
	}

	write32 (nMailBoxClear, 1 << nIPI);
	DataSyncBarrier ();

	s_pThis->IPIHandler (nCore, nIPI);

	return TRUE;
}

#else

void CMultiCoreSupport::LocalInterruptHandler (unsigned nFromCore, unsigned nIPI)
{
	if (s_pThis != 0)
	{
		s_pThis->IPIHandler (ThisCore (), nIPI);
	}
}

#endif

void CMultiCoreSupport::EntrySecondary (void)
{
	s_pThis->m_pMemorySystem->InitializeSecondary ();
	
	unsigned nCore = ThisCore ();
#if AARCH == 32
	write32 (ARM_LOCAL_MAILBOX3_CLR0 + 0x10 * nCore, 0);
#elif RASPPI <= 4
	TSpinTable * volatile pSpinTable = (TSpinTable * volatile) ARM_SPIN_TABLE_BASE;
	pSpinTable->SpinCore[nCore] = 0;
	DataSyncBarrier ();
#else
	CTimer::SimpleMsDelay (20);

	s_pThis->m_bCoreStarted[nCore] = TRUE;
	DataSyncBarrier ();
#endif

#if RASPPI <= 3
	write32 (ARM_LOCAL_MAILBOX_INT_CONTROL0 + 4 * nCore, 1);
#else
	CInterruptSystem::InitializeSecondary ();
#endif
	EnableIRQs ();

	CLogger::Get ()->Write (FromMultiCore, LogDebug, "CPU core %u started", nCore);

	s_pThis->Run (nCore);

	CLogger::Get ()->Write (FromMultiCore, LogDebug, "CPU core %u will halt now", nCore);
}

void main_secondary (void)
{
	CMultiCoreSupport::EntrySecondary ();
}

#endif
