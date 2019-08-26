//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/usbmassdevice.h>
#include <circle/macros.h>

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

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_USBHCI (&m_Interrupt, &m_Timer)
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

	CDevice *pUMSD1 = m_DeviceNameService.GetDevice ("umsd1", TRUE);
	if (pUMSD1 == 0)
	{
		m_Logger.Write (FromKernel, LogError, "USB mass storage device not found");

		return ShutdownHalt;
	}

	u64 ullOffset = 0 * UMSD_BLOCK_SIZE;
	if (pUMSD1->Seek (ullOffset) != ullOffset)
	{
		m_Logger.Write (FromKernel, LogError, "Seek error");

		return ShutdownHalt;
	}

	TMasterBootRecord MBR;
	if (pUMSD1->Read (&MBR, sizeof MBR) != (int) sizeof MBR)
	{
		m_Logger.Write (FromKernel, LogError, "Read error");

		return ShutdownHalt;
	}

	if (MBR.BootSignature != BOOT_SIGNATURE)
	{
		m_Logger.Write (FromKernel, LogError, "Boot signature not found");

		return ShutdownHalt;
	}

	m_Logger.Write (FromKernel, LogNotice, "Dumping the partition table");
	m_Logger.Write (FromKernel, LogNotice, "# Status Type  1stSector    Sectors");

	for (unsigned nPartition = 0; nPartition < 4; nPartition++)
	{
		m_Logger.Write (FromKernel, LogNotice, "%u %02X     %02X   %10u %10u",
				nPartition+1,
				(unsigned) MBR.Partition[nPartition].Status,
				(unsigned) MBR.Partition[nPartition].Type,
				MBR.Partition[nPartition].LBAFirstSector,
				MBR.Partition[nPartition].NumberOfSectors);
	}

	return ShutdownHalt;
}
