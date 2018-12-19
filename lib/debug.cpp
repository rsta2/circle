//
// debug.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2018  R. Stange <rsta2@o2online.de>
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
#include <circle/debug.h>
#include <circle/logger.h>
#include <circle/string.h>
#include <circle/sysconfig.h>
#include <circle/gpiopin.h>

#ifndef NDEBUG

static const char FromDebug[] = "debug";

#ifdef DEBUG_CLICK

static CGPIOPin AudioLeft (GPIOPinAudioLeft, GPIOModeOutput);
static CGPIOPin AudioRight (GPIOPinAudioRight, GPIOModeOutput);

#endif

void debug_hexdump (const void *pStart, unsigned nBytes, const char *pSource)
{
	u8 *pOffset = (u8 *) pStart;

	if (pSource == 0)
	{
		pSource = FromDebug;
	}

	CLogger::Get ()->Write (pSource, LogDebug, "Dumping 0x%X bytes starting at 0x%lX", nBytes,
				(unsigned long) (uintptr) pOffset);
	
	while (nBytes > 0)
	{
		CLogger::Get ()->Write (pSource, LogDebug,
				"%04X: %02X %02X %02X %02X %02X %02X %02X %02X-%02X %02X %02X %02X %02X %02X %02X %02X",
				(unsigned) (uintptr) pOffset & 0xFFFF,
				(unsigned) pOffset[0],  (unsigned) pOffset[1],  (unsigned) pOffset[2],  (unsigned) pOffset[3],
				(unsigned) pOffset[4],  (unsigned) pOffset[5],  (unsigned) pOffset[6],  (unsigned) pOffset[7],
				(unsigned) pOffset[8],  (unsigned) pOffset[9],  (unsigned) pOffset[10], (unsigned) pOffset[11],
				(unsigned) pOffset[12], (unsigned) pOffset[13], (unsigned) pOffset[14], (unsigned) pOffset[15]);

		pOffset += 16;

		if (nBytes >= 16)
		{
			nBytes -= 16;
		}
		else
		{
			nBytes = 0;
		}
	}
}

void debug_stacktrace (const uintptr *pStackPtr, const char *pSource)
{
	if (pSource == 0)
	{
		pSource = FromDebug;
	}
	
	for (unsigned i = 0; i < 64; i++, pStackPtr++)
	{
		extern unsigned char _etext;

		if (   *pStackPtr >= MEM_KERNEL_START
		    && *pStackPtr < (uintptr) &_etext
		    && (*pStackPtr & 3) == 0)
		{
			CLogger::Get ()->Write (pSource, LogDebug, "stack[%u] is 0x%lX", i, (unsigned long) *pStackPtr);
		}
	}
}

#ifdef DEBUG_CLICK

void debug_click (unsigned nMask)
{
	if (nMask & DEBUG_CLICK_LEFT)
	{
		AudioLeft.Invert ();
	}

	if (nMask & DEBUG_CLICK_RIGHT)
	{
		AudioRight.Invert ();
	}
}

#endif

#endif
