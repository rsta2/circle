//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2015  R. Stange <rsta2@o2online.de>
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

#define PARTITION	"emmc1-1"
#define FILENAME	"circle.txt"

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_EMMC (&m_Interrupt, &m_Timer, &m_ActLED)
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

	if (bOK)
	{
		bOK = m_Interrupt.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Timer.Initialize ();
	}

	if (bOK)
	{
		bOK = m_EMMC.Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	// Mount file system
	CDevice *pPartition = m_DeviceNameService.GetDevice (PARTITION, TRUE);
	if (pPartition == 0)
	{
		m_Logger.Write (FromKernel, LogPanic, "Partition not found: %s", PARTITION);
	}

	if (!m_FileSystem.Mount (pPartition))
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot mount partition: %s", PARTITION);
	}

	// Show contents of root directory
	TDirentry Direntry;
	TFindCurrentEntry CurrentEntry;
	unsigned nEntry = m_FileSystem.RootFindFirst (&Direntry, &CurrentEntry);
	for (unsigned i = 0; nEntry != 0; i++)
	{
		if (!(Direntry.nAttributes & FS_ATTRIB_SYSTEM))
		{
			CString FileName;
			FileName.Format ("%-14s", Direntry.chTitle);

			m_Screen.Write ((const char *) FileName, FileName.GetLength ());

			if (i % 5 == 4)
			{
				m_Screen.Write ("\n", 1);
			}
		}

		nEntry = m_FileSystem.RootFindNext (&Direntry, &CurrentEntry);
	}
	m_Screen.Write ("\n", 1);

	// Create file and write to it
	unsigned hFile = m_FileSystem.FileCreate (FILENAME);
	if (hFile == 0)
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot create file: %s", FILENAME);
	}

	for (unsigned nLine = 1; nLine <= 5; nLine++)
	{
		CString Msg;
		Msg.Format ("Hello File! (Line %u)\n", nLine);

		if (m_FileSystem.FileWrite (hFile, (const char *) Msg, Msg.GetLength ()) != Msg.GetLength ())
		{
			m_Logger.Write (FromKernel, LogError, "Write error");
			break;
		}
	}

	if (!m_FileSystem.FileClose (hFile))
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot close file");
	}

	// Reopen file, read it and display its contents
	hFile = m_FileSystem.FileOpen (FILENAME);
	if (hFile == 0)
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot open file: %s", FILENAME);
	}

	char Buffer[100];
	unsigned nResult;
	while ((nResult = m_FileSystem.FileRead (hFile, Buffer, sizeof Buffer)) > 0)
	{
		if (nResult == FS_ERROR)
		{
			m_Logger.Write (FromKernel, LogError, "Read error");
			break;
		}

		m_Screen.Write (Buffer, nResult);
	}
	
	if (!m_FileSystem.FileClose (hFile))
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot close file");
	}

	return ShutdownHalt;
}
