//
// chainboot.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2019  R. Stange <rsta2@o2online.de>
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
#include <circle/chainboot.h>
#include <circle/synchronize.h>
#include <circle/sysconfig.h>
#include <circle/util.h>
#include <assert.h>

typedef void TKernelStart (void);
typedef void TChainBootStub (const void *, size_t);

static const void *s_pKernelImage = 0;
static size_t s_nKernelSize = 0;

#define STUB_MAX_SIZE	0x400
static void ChainBootStub (const void *pKernelImage, size_t nKernelSize)
{
	const u32 *pSrc = (const u32 *) pKernelImage;
	u32 *pDest = (u32 *) MEM_KERNEL_START;

	nKernelSize += sizeof (u32)-1;
	nKernelSize /= sizeof (u32);

	while (nKernelSize--)
	{
		*pDest++ = *pSrc++;
	}

	// all implemented as macros (must not call into overwritten code)
	InvalidateInstructionCache ();
#if AARCH == 32
	FlushBranchTargetCache ();
#endif
	DataSyncBarrier ();
	InstructionSyncBarrier ();

	TKernelStart *pKernelStart = (TKernelStart *) MEM_KERNEL_START;
	(*pKernelStart) ();
}

void EnableChainBoot (const void *pKernelImage, size_t nKernelSize)
{
#ifdef ARM_ALLOW_MULTI_CORE
	assert (0);		// not supported with multi-core
#endif

	s_pKernelImage = pKernelImage;
	s_nKernelSize = nKernelSize;
}

boolean IsChainBootEnabled (void)
{
	return s_pKernelImage != 0 ? TRUE : FALSE;
}

void DoChainBoot (void)
{
	DisableFIQs ();
	DisableIRQs ();

	TChainBootStub *pStub = (TChainBootStub *) (MEM_KERNEL_START - STUB_MAX_SIZE);

	memcpy ((void *) pStub, (const void *) &ChainBootStub, STUB_MAX_SIZE);

	InvalidateInstructionCache ();
#if AARCH == 32
	FlushBranchTargetCache ();
#endif
	DataSyncBarrier ();
	InstructionSyncBarrier ();

#if AARCH == 64
	asm volatile ("hvc #0");		// return to EL2h mode
#endif

	(*pStub) (s_pKernelImage, s_nKernelSize);
}
