//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2025  R. Stange <rsta2gmx.net>
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
#include "ledtask.h"
#include <circle/string.h>
#include <circle/synchronize.h>
#include <circle/macros.h>

#define DRIVE		"NVME:"		// for FatFs
#define PARTITION	"nvme1-1"	// for Circle FATFS

#if TEST_RO
	#define FILENAME	"issue.txt"
#else
	#define FILENAME	"circle.txt"
#endif

#if TESTSEL == TEST_DUMP_MBR

struct TCHSAddress
{
	unsigned char Head;
	unsigned char Sector	   : 6,
		      CylinderHigh : 2;
	unsigned char CylinderLow;
}
PACKED;

struct TPartitionEntry
{
	unsigned char	Status;
	TCHSAddress	FirstSector;
	unsigned char	Type;
	TCHSAddress	LastSector;
	unsigned	LBAFirstSector;
	unsigned	NumberOfSectors;
}
PACKED;

struct TMasterBootRecord
{
	unsigned char	BootCode[0x1BE];
	TPartitionEntry	Partition[4];
	unsigned short	BootSignature;
	#define BOOT_SIGNATURE		0xAA55
}
PACKED;

#endif

LOGMODULE ("kernel");

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_pTarget (nullptr),
	m_NVMe (&m_Interrupt)
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
		m_pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
		if (!m_pTarget)
		{
			m_pTarget = &m_Screen;
		}

		bOK = m_Logger.Initialize (m_pTarget);
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
		bOK = m_NVMe.Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	LOGNOTE ("Compile time: " __DATE__ " " __TIME__);

#if TESTSEL == TEST_DUMP_MBR
	////////////////////////////////////////////////////////////////////////
	m_NVMe.Seek (0);

	DMA_BUFFER (u8, Buffer, 512);
	int nResult = m_NVMe.Read (Buffer, sizeof Buffer);
        if (nResult != sizeof Buffer)
	{
		LOGPANIC ("Read failed (%d)", nResult);
        }

	TMasterBootRecord *pMBR = reinterpret_cast<TMasterBootRecord *> (Buffer);
	if (pMBR->BootSignature != BOOT_SIGNATURE)
	{
		LOGPANIC("Boot signature not found (0x%X)", (unsigned) pMBR->BootSignature);
	}

	LOGNOTE ("Dumping the partition table");
	LOGNOTE ("# Status Type  1stSector    Sectors");

	for (unsigned nPartition = 0; nPartition < 4; nPartition++)
	{
		LOGNOTE ("%u %02X     %02X   %10u %10u",
				nPartition+1,
				(unsigned) pMBR->Partition[nPartition].Status,
				(unsigned) pMBR->Partition[nPartition].Type,
				pMBR->Partition[nPartition].LBAFirstSector,
				pMBR->Partition[nPartition].NumberOfSectors);
	}
#elif TESTSEL == TEST_RAW_READ
	////////////////////////////////////////////////////////////////////////
#ifdef NO_BUSY_WAIT
	new CLEDTask (&m_ActLED);
#endif

	const unsigned BlocksToRead = 100000;
	const size_t BlockSize = 4096;

	unsigned nStartTicks = m_Timer.GetClockTicks ();
        for (unsigned nBlock = 0; nBlock < BlocksToRead; nBlock++)
	{
		if (nBlock % (BlocksToRead / 10) == 0)
		{
			CString Msg;
			Msg.Format ("%u\r", nBlock);
			m_pTarget->Write (Msg, Msg.GetLength ());
		}

		m_NVMe.Seek (nBlock * BlockSize);

		DMA_BUFFER (u8, Buffer, BlockSize);
		int nResult = m_NVMe.Read (Buffer, sizeof Buffer);
		if (nResult != sizeof Buffer)
		{
			LOGPANIC("Read failed (%d)", nResult);
		}
	}

	unsigned nEndTicks = m_Timer.GetClockTicks ();

	LOGNOTE("Transfer rate was %.1f MBytes/sec",
		  (double) (BlocksToRead * BlockSize) / 0x100000
		/ (nEndTicks - nStartTicks) * CLOCKHZ);
#elif TESTSEL == TEST_CIRCLE_FS
	////////////////////////////////////////////////////////////////////////
	CDevice *pPartition = m_DeviceNameService.GetDevice (PARTITION, TRUE);
	if (!pPartition)
	{
		LOGPANIC ("Partition not found: %s", PARTITION);
	}

	if (!m_FileSystem.Mount (pPartition))
	{
		LOGPANIC ("Cannot mount partition: %s", PARTITION);
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

			m_pTarget->Write (FileName.c_str (), FileName.GetLength ());

			if (i % 5 == 4)
			{
				m_pTarget->Write ("\n", 1);
			}
		}

		nEntry = m_FileSystem.RootFindNext (&Direntry, &CurrentEntry);
	}
	m_pTarget->Write ("\n", 1);

	unsigned hFile;
#if !TEST_RO
	// Create file and write to it
	hFile = m_FileSystem.FileCreate (FILENAME);
	if (hFile == 0)
	{
		LOGPANIC ("Cannot create file: %s", FILENAME);
	}

	for (unsigned nLine = 1; nLine <= 5; nLine++)
	{
		CString Msg;
		Msg.Format ("Hello File! (Line %u)\n", nLine);

		if (m_FileSystem.FileWrite (hFile, Msg.c_str(), Msg.GetLength()) != Msg.GetLength ())
		{
			LOGERR ("Write error");
			break;
		}
	}

	if (!m_FileSystem.FileClose (hFile))
	{
		LOGPANIC ("Cannot close file");
	}

	// Circle FATFS does not sync the disk on its own
	if (m_NVMe.IOCtl (DEVICE_IOCTL_SYNC, nullptr) < 0)
	{
		LOGPANIC ("Sync failed");
	}
#endif

	// Reopen file, read it and display its contents
	hFile = m_FileSystem.FileOpen (FILENAME);
	if (hFile == 0)
	{
		LOGPANIC ("Cannot open file: %s", FILENAME);
	}

	char Buffer[100];
	unsigned nResult;
	while ((nResult = m_FileSystem.FileRead (hFile, Buffer, sizeof Buffer)) > 0)
	{
		if (nResult == FS_ERROR)
		{
			LOGERR ("Read error");
			break;
		}

		m_pTarget->Write (Buffer, nResult);
	}

	if (!m_FileSystem.FileClose (hFile))
	{
		LOGPANIC ("Cannot close file");
	}
#elif TESTSEL == TEST_FATFS
	////////////////////////////////////////////////////////////////////////
	// Mount file system
	FRESULT Result = f_mount (&m_FileSystem, DRIVE, 1);
	if (Result != FR_OK)
	{
		LOGPANIC ("Cannot mount drive: %s (%d)", DRIVE, Result);
	}

	// Show contents of root directory
	DIR Directory;
	FILINFO FileInfo;
	Result = f_findfirst (&Directory, &FileInfo, DRIVE "/", "*");
	for (unsigned i = 0; Result == FR_OK && FileInfo.fname[0]; i++)
	{
		if (!(FileInfo.fattrib & (AM_HID | AM_SYS)))
		{
			CString FileName;
			FileName.Format ("%-25s ", FileInfo.fname);

			m_pTarget->Write (FileName.c_str (), FileName.GetLength ());

			if (i % 3 == 2)
			{
				m_pTarget->Write ("\n", 1);
			}
		}

		Result = f_findnext (&Directory, &FileInfo);
	}
	m_pTarget->Write ("\n", 1);

	FIL File;
#if !TEST_RO
	// Create file and write to it
	Result = f_open (&File, DRIVE "/" FILENAME, FA_WRITE | FA_CREATE_ALWAYS);
	if (Result != FR_OK)
	{
		LOGPANIC ("Cannot create file: %s", FILENAME);
	}

	for (unsigned nLine = 1; nLine <= 5; nLine++)
	{
		CString Msg;
		Msg.Format ("Hello File! (Line %u)\n", nLine);

		unsigned nBytesWritten;
		if (   f_write (&File, Msg.c_str (), Msg.GetLength (), &nBytesWritten) != FR_OK
		    || nBytesWritten != Msg.GetLength ())
		{
			LOGERR ("Write error");
			break;
		}
	}

	if (f_close (&File) != FR_OK)
	{
		LOGPANIC ("Cannot close file");
	}
#endif

	// Reopen file, read it and display its contents
	Result = f_open (&File, DRIVE "/" FILENAME, FA_READ | FA_OPEN_EXISTING);
	if (Result != FR_OK)
	{
		LOGPANIC ("Cannot open file: %s", FILENAME);
	}

	char Buffer[100];
	unsigned nBytesRead;
	while ((Result = f_read (&File, Buffer, sizeof Buffer, &nBytesRead)) == FR_OK)
	{
		if (nBytesRead > 0)
		{
			m_pTarget->Write (Buffer, nBytesRead);
		}

		if (nBytesRead < sizeof Buffer)		// EOF?
		{
			break;
		}
	}

	if (Result != FR_OK)
	{
		LOGERR ("Read error");
	}
	
	if (f_close (&File) != FR_OK)
	{
		LOGPANIC ("Cannot close file");
	}

	// Unmount file system
	if (f_mount (0, DRIVE, 0) != FR_OK)
	{
		LOGPANIC ("Cannot unmount drive: %s", DRIVE);
	}
#else
	#error Select a test in kernel.h!
#endif

	// m_NVMe.DumpStatus ();

#ifdef POWER_OFF_ON_HALT
	m_Timer.MsDelay (1000);		// Time for NVMe to finish
#endif

	return ShutdownHalt;
}
