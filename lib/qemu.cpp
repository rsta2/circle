//
// qemu.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2020-2021  R. Stange <rsta2@o2online.de>
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
#include <circle/qemu.h>

#define SHFB_MAGIC_0	0x53
#define SHFB_MAGIC_1	0x48
#define SHFB_MAGIC_2	0x46
#define SHFB_MAGIC_3	0x42

#define ADP_STOPPED_INTERNAL_ERROR	0x20024
#define ADP_STOPPED_APPLICATION_EXIT	0x20026

TSemihostingValue CallSemihostingSingle (u32 nOperation, TSemihostingValue nParam)
{
	TSemihostingValue nResult;

#if AARCH == 32
	asm volatile
	(
		"mov r0, %1\n"
		"mov r1, %2\n"
		"svc 0x123456\n"
		"mov %0, r0\n"

		: "=r" (nResult) : "r" (nOperation), "r" (nParam) : "r0", "r1"
	);
#else
	u64 ulOperation = nOperation;
	asm volatile
	(
		"mov x0, %1\n"
		"mov x1, %2\n"
		"hlt #0xF000\n"
		"mov %0, x0\n"

		: "=r" (nResult) : "r" (ulOperation), "r" (nParam) : "x0", "x1"
	);
#endif

	return nResult;
}

TSemihostingValue CallSemihosting (u32 nOperation,
				   TSemihostingValue nParam1, TSemihostingValue nParam2,
				   TSemihostingValue nParam3, TSemihostingValue nParam4)
{
	volatile TSemihostingValue Data[4] = {nParam1, nParam2, nParam3, nParam4};
	TSemihostingValue nResult;

#if AARCH == 32
	asm volatile
	(
		"mov r0, %1\n"
		"mov r1, %2\n"
		"svc 0x123456\n"
		"mov %0, r0\n"

		: "=r" (nResult) : "r" (nOperation), "r" ((uintptr) &Data) : "r0", "r1"
	);
#else
	u64 ulOperation = nOperation;
	asm volatile
	(
		"mov x0, %1\n"
		"mov x1, %2\n"
		"hlt #0xF000\n"
		"mov %0, x0\n"

		: "=r" (nResult) : "r" (ulOperation), "r" ((uintptr) &Data) : "x0", "x1"
	);
#endif

	return nResult;
}

boolean SemihostingFeatureSupported (u32 nFeature)
{
	static const char FileName[] = ":semihosting-features";
	TSemihostingValue nHandle = CallSemihosting (SEMIHOSTING_SYS_OPEN,  (uintptr) FileName,
						     SEMIHOSTING_OPEN_READ_BIN, sizeof FileName-1);
	if (nHandle == SEMIHOSTING_NO_HANDLE)
	{
		return FALSE;
	}

	boolean bResult = FALSE;

	u8 Buffer[5] = {0};
	if (   CallSemihosting (SEMIHOSTING_SYS_READ, nHandle, (uintptr) Buffer, sizeof Buffer) == 0
	    && Buffer[0] == SHFB_MAGIC_0
	    && Buffer[1] == SHFB_MAGIC_1
	    && Buffer[2] == SHFB_MAGIC_2
	    && Buffer[3] == SHFB_MAGIC_3
	    && (Buffer[4] & nFeature) != 0)
	{
		bResult = TRUE;
	}

	CallSemihosting (SEMIHOSTING_SYS_CLOSE, nHandle);

	return bResult;
}

void SemihostingExit (TSemihostingValue nStatusCode)
{
	if (SemihostingFeatureSupported (SEMIHOSTING_EXT_EXIT_EXTENDED))
	{
		CallSemihosting (SEMIHOSTING_SYS_EXIT_EXTENDED, ADP_STOPPED_APPLICATION_EXIT,
				 nStatusCode);
	}
	else
	{
#if AARCH == 32
		CallSemihostingSingle (SEMIHOSTING_SYS_EXIT,
				         nStatusCode == 0
				       ? ADP_STOPPED_APPLICATION_EXIT
				       : ADP_STOPPED_INTERNAL_ERROR);
#else
		CallSemihosting (SEMIHOSTING_SYS_EXIT, ADP_STOPPED_APPLICATION_EXIT, nStatusCode);
#endif
	}
}
