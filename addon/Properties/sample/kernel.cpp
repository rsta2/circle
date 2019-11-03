//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2018  R. Stange <rsta2@o2online.de>
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
#ifdef USE_FATFS
	#include <Properties/propertiesfatfsfile.h>
#else
	#include <Properties/propertiesfile.h>
#endif

#ifdef USE_FATFS
	#define DRIVE		"SD:"
	#define FILENAME	"/params.txt"
#else
	#define PARTITION	"emmc1-1"
	#define FILENAME	"params.txt"
#endif

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

#ifdef USE_FATFS
	// Mount file system
	if (f_mount (&m_FileSystem, DRIVE, 1) != FR_OK)
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot mount drive: %s", DRIVE);
	}

	CPropertiesFatFsFile Properties (DRIVE FILENAME, &m_FileSystem);
#else
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

	CPropertiesFile Properties (FILENAME, &m_FileSystem);
#endif

	if (!Properties.Load ())
	{
		m_Logger.Write (FromKernel, LogPanic, "Error loading properties from %s (line %u)",
				FILENAME, Properties.GetErrorLine ());
	}

#ifndef NDEBUG
	Properties.Dump ();
#endif

	// display some properties
	m_Logger.Write (FromKernel, LogNotice, "logdev is \"%s\"",
			Properties.GetString ("logdev", "tty1"));

	m_Logger.Write (FromKernel, LogNotice, "loglevel is %u",
			Properties.GetNumber ("loglevel", 3));

	m_Logger.Write (FromKernel, LogNotice, "usbpowerdelay is %u",
			Properties.GetNumber ("usbpowerdelay", 510));	// unset in params.txt by default

	// set or overwrite some properties
	Properties.SetString ("logdev", "tty1");
	Properties.SetNumber ("loglevel", 3);
	Properties.SetNumber ("usbpowerdelay", 1000);

	if (!Properties.Save ())
	{
		m_Logger.Write (FromKernel, LogPanic, "Error saving properties to %s", FILENAME);
	}

	return ShutdownHalt;
}
