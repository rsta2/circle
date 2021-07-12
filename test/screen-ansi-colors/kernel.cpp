//
// kernel.cpp
//
// This file:
//	Copyright (C) 2021  D. Rimron
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
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
#include <circle/timer.h>
#include <assert.h>

static const char FromKernel[] = "kernel";

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

TShutdownMode CKernel::Run (void)
{
	CString Message;
	
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	m_Screen.Write ("\u001b[7mCircle ANSI Code Test\u001b[0m\n", 30);

	// Normal Backgrounds
	for (u8 uColumn = 40; uColumn <= 47 ; uColumn++)
	{
		Message.Format("\u001b[%dmBG:%3d : ", uColumn, uColumn);

		m_Screen.Write((const char *)Message, Message.GetLength());
		// Normal Columns
		for (u8 uRow = 30; uRow <= 37 ; uRow++)
		{
			Message.Format("\u001b[%dm %d", uRow, uRow);

			m_Screen.Write((const char *)Message, Message.GetLength());
		}
		
		// Bright Columns
		for (u8 uRow = 90; uRow <= 97 ; uRow++)
		{
			Message.Format("\u001b[%dm %d", uRow, uRow);

			m_Screen.Write((const char *)Message, Message.GetLength());
		}
		m_Screen.Write ("\n", 1);
	}

	// Bright Backgrounds
	for (u8 uColumn = 100; uColumn <= 107 ; uColumn++)
	{
		Message.Format("\u001b[%dmBG:%3d : ", uColumn, uColumn);

		m_Screen.Write((const char *)Message, Message.GetLength());
		// Normal Columns
		for (u8 uRow = 30; uRow <= 37 ; uRow++)
		{
			Message.Format("\u001b[%dm %d", uRow, uRow);

			m_Screen.Write((const char *)Message, Message.GetLength());
		}
		
		// Bright Columns
		for (u8 uRow = 90; uRow <= 97 ; uRow++)
		{
			Message.Format("\u001b[%dm %d", uRow, uRow);

			m_Screen.Write((const char *)Message, Message.GetLength());
		}
		m_Screen.Write ("\n", 1);
	}

	while(1)
	{
		for (u8 uRow = 1; uRow <= 20 ; uRow++)
		{
			for (u8 uColumn = 1; uColumn <= 60 ; uColumn++)
			{
				Message.Format("\u001b[%d;%dH", uRow, uColumn);
				m_Screen.Write((const char *)Message, Message.GetLength());

				CTimer::SimpleMsDelay (100);
			}
		}
	}

	return ShutdownHalt;
}
