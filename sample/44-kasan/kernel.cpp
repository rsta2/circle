//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2025  Stephan Mühlstrasser
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
#include "kernel.h"
#include <circle/string.h>
#include <circle/debug.h>
#include <circle/util.h>
#include <circle/alloc.h>
#include <assert.h>

static const char FromKernel[] = "kasan-demo";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Logger (m_Options.GetLogLevel ())
{
	m_ActLED.Blink (5);	// show we are alive
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;

	if (bOK)
	{
		bOK = m_Screen.Initialize ();
	}
	
	if (bOK)
	{
		bOK = m_Serial.Initialize (115200);
	}
	
	if (bOK)
	{
		CDevice *pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
		if (pTarget == 0)
		{
			pTarget = &m_Screen;
		}

		bOK = m_Logger.Initialize (pTarget);
	}
	
	return bOK;
}

namespace {

	int test_stack(CLogger& logger)
	{
		char stack_array[11];

		logger.Write (FromKernel, LogNotice, "Out-of-bounds access on stack, array address %p", stack_array);

		memset(stack_array, 0, 12);
		char const c = stack_array[11];
		int const result = c == stack_array[-1];

		return result;
	}

	int test_heap_malloc(CLogger& logger)
	{
		char *p = reinterpret_cast<char *>(malloc(10));
		if (!p)
		{
			logger.Write (FromKernel, LogError, "Out of memory in malloc()");
			return -1;
		}

		logger.Write (FromKernel, LogNotice, "Out-of-bounds access on heap, heap address %p", p);
		p[3] = 7;
		p[11] = 8;
		
		p = reinterpret_cast<char *>(realloc(p, 2));
		if (!p)
		{
			logger.Write (FromKernel, LogError, "Out of memory in realloc()");
			return -1;
		}
		// Access to p[3] no longer allowed.
		int result = p[3];

		free(p);

		// No longer allowed.
		result += p[0];

		return result;
	}

	int test_heap_new(CLogger& logger)
	{
		struct s_t
		{
			int a;
			char b[15];
		};

		s_t * const s = new s_t[11];

		logger.Write (FromKernel, LogNotice, "Out-of-bounds access on heap, heap address %p", s);

		// Ok.
		s[3].b[9] = 3;
		// Not ok.
		s[11].a = 4;
		
		delete [] s;
		int result = s[3].b[9];

		return result;
	}

	int test_static(CLogger& logger)
	{
		static char static_buffer[10];

		logger.Write (FromKernel, LogNotice, "Out-of-bounds access on static memory, address %p", static_buffer);

		memset(static_buffer, 0, sizeof(static_buffer) + 2);
		char const c = static_buffer[11];

		// The following out-of-bounds access before the start of the array
		// should be detected by KASan, but currently is not.
		logger.Write (FromKernel, LogNotice, "Out-of-bounds access on static memory before start of array, address %p (undetected)", &static_buffer[-3]);
		int const result = c == static_buffer[-3];

		return result;
	}
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	int const a = test_stack(m_Logger);

	int const b = test_heap_malloc(m_Logger);

	int const c = test_heap_new(m_Logger);

	int const d = test_static(m_Logger);

	// The magic value is just to have some computation with the results,
	// so that the compiler does not optimize away the tests.
	int const magic = a + b + c + d;
	m_Logger.Write (FromKernel, LogNotice,
		"Address Sanitizer test finished", magic);

	return ShutdownHalt;
}
