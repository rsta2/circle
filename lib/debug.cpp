//
// debug.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2025  R. Stange <rsta2@gmx.net>
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

static const char FromDebug[] = "debug";

#ifdef DEBUG_CLICK

static CGPIOPin AudioLeft (GPIOPinAudioLeft, GPIOModeOutput);
static CGPIOPin AudioRight (GPIOPinAudioRight, GPIOModeOutput);

#endif

void DebugHexDump (const void *pStart, unsigned nBytes, const char *pSource, unsigned nFlags)
{
	u8 *pOffset = (u8 *) pStart;

	if (pSource == 0)
	{
		pSource = FromDebug;
	}

	if (nFlags & DEBUG_HEXDUMP_HEADER)
	{
		CLogger::Get ()->Write (pSource, LogDebug,
				"Dumping 0x%X bytes starting at 0x%lX", nBytes,
				(uintptr) pOffset);
	}
	
	while (nBytes > 0)
	{
		CString Hex;
		CString Ascii;
		for (unsigned i = 0; i < 16;)
		{
			CString Data;
			Data.Format ("%c%02X", i == 8 ? '-' : ' ', pOffset[i]);
			Hex.Append (Data);

			if (nFlags & DEBUG_HEXDUMP_ASCII)
			{
				char chChar = (char) pOffset[i];
				if (!(' ' <= chChar && chChar <= '~'))
				{
					chChar = '.';
				}

				Ascii.Append (chChar);
			}

			if (++i >= nBytes)
			{
				break;
			}
		}

		if (   (nFlags & DEBUG_HEXDUMP_ASCII)
		    && nBytes < 16)
		{
			for (int i = 16 - nBytes ; i > 0; i--)
			{
				Hex.Append ("   ");
			}
		}

		unsigned long ulPrefix = (uintptr) pOffset;
		if (!(nFlags & DEBUG_HEXDUMP_ADDRESS))
		{
			 ulPrefix -= (uintptr) pStart;
		}

		CLogger::Get ()->Write (pSource, LogDebug, "%04lX:%s %s",
					ulPrefix & 0xFFFFUL, Hex.c_str (), Ascii.c_str ());

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

void DebugStackTrace (const uintptr *pStackPtr, const char *pSource)
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

void DebugClick (unsigned nMask)
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
