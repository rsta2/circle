//
// multicore.cpp
//
// Some information in this file is taken from Linux and is:
//	Copyright (C) Broadcom
//	Licensed under GPL2
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2016  R. Stange <rsta2@o2online.de>
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
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/memio.h>
#include <circle/synchronize.h>
#include <assert.h>

static const char FromMultiCore[] = "mcore";

CMultiCoreSupport *CMultiCoreSupport::s_pThis = 0;

CMultiCoreSupport::CMultiCoreSupport (CMemorySystem *pMemorySystem)
:	m_pMemorySystem (pMemorySystem)
{
	assert (s_pThis == 0);
	s_pThis = this;
}

CMultiCoreSupport::~CMultiCoreSupport (void)
{
	m_pMemorySystem = 0;

	s_pThis = 0;
}

boolean CMultiCoreSupport::Initialize (void)
{
	u32 nRouting = read32 (ARM_LOCAL_GPU_INT_ROUTING);
	nRouting &= ~0x0C;
	nRouting |= 1 << 2;
	write32 (ARM_LOCAL_GPU_INT_ROUTING, nRouting);

	write32 (ARM_LOCAL_MAILBOX_INT_CONTROL0, 1);		// enable IPI on core 0

	for (unsigned nCore = 1; nCore < CORES; nCore++)
	{
		u32 nMailBoxClear = ARM_LOCAL_MAILBOX3_CLR0 + 0x10 * nCore;

		DataSyncBarrier ();

		unsigned nTimeout = 100;
		while (read32 (nMailBoxClear) != 0)
		{
			if (--nTimeout == 0)
			{
				CLogger::Get ()->Write (FromMultiCore, LogError, "CPU core %u does not respond", nCore);

				return FALSE;
			}

			CTimer::SimpleMsDelay (1);
		}

		write32 (ARM_LOCAL_MAILBOX3_SET0 + 0x10 * nCore, (u32) &_start_secondary);

		nTimeout = 100;
		while (read32 (nMailBoxClear) != 0)
		{
			if (--nTimeout == 0)
			{
				CLogger::Get ()->Write (FromMultiCore, LogError, "CPU core %u did not start", nCore);

				return FALSE;
			}

			CTimer::SimpleMsDelay (1);
		}
	}

	return TRUE;
}

void CMultiCoreSupport::IPIHandler (unsigned nCore, unsigned nIPI)
{
	assert (nCore < CORES);
	assert (nIPI < 32);

	if (nIPI == IPI_HALT_CORE)
	{
		CLogger::Get ()->Write (FromMultiCore, LogDebug, "CPU core %u will halt now", nCore);

		halt ();
	}
}

void CMultiCoreSupport::SendIPI (unsigned nCore, unsigned nIPI)
{
	assert (nCore < CORES);
	assert (nIPI < 32);

	write32 (ARM_LOCAL_MAILBOX0_SET0 + 0x10 * nCore, 1 << nIPI);
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

	u32 nMailBoxClear = ARM_LOCAL_MAILBOX0_CLR0 + 0x10 * nCore;
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

unsigned CMultiCoreSupport::ThisCore (void)
{
	u32 nMPIDR;
	asm volatile ("mrc p15, 0, %0, c0, c0, 5" : "=r" (nMPIDR));

	return nMPIDR & (CORES-1);
}

void CMultiCoreSupport::EntrySecondary (void)
{
	assert (s_pThis != 0);

	assert (s_pThis->m_pMemorySystem != 0);
	s_pThis->m_pMemorySystem->InitializeSecondary ();
	
	unsigned nCore = ThisCore ();
	write32 (ARM_LOCAL_MAILBOX3_CLR0 + 0x10 * nCore, 0);

	write32 (ARM_LOCAL_MAILBOX_INT_CONTROL0 + 4 * nCore, 1);
	EnableInterrupts ();

	CLogger::Get ()->Write (FromMultiCore, LogDebug, "CPU core %u started", nCore);

	s_pThis->Run (nCore);

	CLogger::Get ()->Write (FromMultiCore, LogDebug, "CPU core %u will halt now", nCore);
}

void main_secondary (void)
{
	CMultiCoreSupport::EntrySecondary ();
}

#endif
