//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
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
#include <circle/device.h>
#include <circle/string.h>
#include <circle/util.h>
#include <assert.h>

#ifdef USE_FATFS
	#define DRIVE		"USB:"		// "USB2:", "USB3:"
	#define DEVICE		"umsd1"		// "umsd2", "umsd3"
#else
	#define DRIVE		"umsd1"
	#define PARTITION	"umsd1-1"
#endif

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_USBHCI (&m_Interrupt, &m_Timer, TRUE),		// TRUE: enable plug-and-play
	m_bStorageAttached (FALSE)
#ifndef USE_FATFS
	, m_pFileSystem (0)
#endif
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
		bOK = m_USBHCI.Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	while (1)
	{
		// USB device detection
		m_Logger.Write (FromKernel, LogNotice, "Plug in an USB flash drive!");

#ifdef USE_FATFS
		while (1)
		{
			// Update the tree of connected USB devices
			if (m_USBHCI.UpdatePlugAndPlay ())
			{
				// Try to mount file system
				FRESULT Result = f_mount (&m_FileSystem, DRIVE, 1);
				if (Result == FR_OK)
				{
					break;
				}
				else if (Result != FR_NOT_READY)
				{
					m_Logger.Write (FromKernel, LogError,
							"Mount error (%u)", Result);
				}
			}
		}

		assert (!m_bStorageAttached);
		m_bStorageAttached = TRUE;

		CDevice *pDevice = m_DeviceNameService.GetDevice (DEVICE, TRUE);
		assert (pDevice != 0);
		pDevice->RegisterRemovedHandler (StorageRemovedHandler, this);

		// Show contents of root directory
		DIR Directory;
		FILINFO FileInfo;
		FRESULT Result = f_findfirst (&Directory, &FileInfo, DRIVE "/", "*");
		for (unsigned i = 0; Result == FR_OK && FileInfo.fname[0]; i++)
		{
			if (!(FileInfo.fattrib & (AM_HID | AM_SYS)))
			{
				// cut too long file names
				if (strlen (FileInfo.fname) > 18)
				{
					strcpy (FileInfo.fname + 15, "...");
				}

				CString FileName;
				FileName.Format ("%-19s", FileInfo.fname);

				m_Screen.Write ((const char *) FileName, FileName.GetLength ());

				if (i % 4 == 3)
				{
					m_Screen.Write ("\n", 1);
				}
			}

			Result = f_findnext (&Directory, &FileInfo);
		}
		m_Screen.Write ("\n", 1);

		// Unmount file system
		if (f_mount (0, DRIVE, 0) != FR_OK)
		{
			m_Logger.Write (FromKernel, LogPanic,
					"Cannot unmount drive: %s", DRIVE);
		}
#else
		while (1)
		{
			// Update the tree of connected USB devices
			if (m_USBHCI.UpdatePlugAndPlay ())
			{
				CDevice *pDevice = m_DeviceNameService.GetDevice (DRIVE, TRUE);
				if (pDevice != 0)
				{
					assert (!m_bStorageAttached);
					m_bStorageAttached = TRUE;

					pDevice->RegisterRemovedHandler (StorageRemovedHandler, this);

					break;
				}
			}
		}

		// Mount file system
		CDevice *pPartition = m_DeviceNameService.GetDevice (PARTITION, TRUE);
		if (   pPartition != 0
		    && (m_pFileSystem = new CFATFileSystem) != 0
		    && m_pFileSystem->Mount (pPartition))
		{
			// Show contents of root directory
			TDirentry Direntry;
			TFindCurrentEntry CurrentEntry;
			unsigned nEntry = m_pFileSystem->RootFindFirst (&Direntry, &CurrentEntry);
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

				nEntry = m_pFileSystem->RootFindNext (&Direntry, &CurrentEntry);
			}
			m_Screen.Write ("\n", 1);

			m_pFileSystem->UnMount ();

			delete m_pFileSystem;
			m_pFileSystem = 0;
		}
		else
		{
			m_Logger.Write (FromKernel, LogError,
					"Cannot mount partition: %s", PARTITION);
		}
#endif

		m_Logger.Write (FromKernel, LogNotice, "Remove USB flash drive!");

		while (1)
		{
			// Update the tree of connected USB devices
			if (m_USBHCI.UpdatePlugAndPlay ())
			{
				if (!m_bStorageAttached)
				{
					break;
				}
			}
		}
	}

	return ShutdownHalt;
}

void CKernel::StorageRemovedHandler (CDevice *pDevice, void *pContext)
{
	CKernel *pThis = (CKernel *) pContext;
	assert (pThis != 0);

	assert (pThis->m_bStorageAttached);
	pThis->m_bStorageAttached = FALSE;
}
