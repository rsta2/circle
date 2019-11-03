//
// partitionmanager.cpp
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
#include <circle/fs/partitionmanager.h>
#include <circle/fs/fsdef.h>
#include <circle/devicenameservice.h>
#include <circle/logger.h>
#include <circle/macros.h>
#include <assert.h>

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
	unsigned int	LBAFirstSector;
	unsigned int	NumberOfSectors;
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

static const char FromPartitionManager[] = "partm";

CPartitionManager::CPartitionManager (CDevice *pDevice, const char *pDeviceName)
:	m_pDevice (pDevice),
	m_DeviceName (pDeviceName)
{
	for (unsigned i = 0; i < MAX_PARTITIONS; i++)
	{
		m_pPartition[i] = 0;
	}
}

CPartitionManager::~CPartitionManager (void)
{
	unsigned nPartition = 0;
	for (unsigned i = 0; i < MAX_PARTITIONS; i++)
	{
		if (m_pPartition[i] != 0)
		{
			CString PartitionName;
			PartitionName.Format ("%s-%u", (const char *) m_DeviceName, ++nPartition);
			CDeviceNameService::Get ()->RemoveDevice (PartitionName, TRUE);

			delete m_pPartition[i];
			m_pPartition[i] = 0;
		}
	}

	m_pDevice = 0;
}

boolean CPartitionManager::Initialize (void)
{
	TMasterBootRecord MBR;
	assert (sizeof MBR == FS_BLOCK_SIZE);

	if (   m_pDevice->Seek (0) != 0
	    || m_pDevice->Read (&MBR, sizeof MBR) != sizeof MBR)
	{
		CLogger::Get ()->Write (FromPartitionManager, LogError, "Cannot read MBR");

		return FALSE;
	}

	if (MBR.BootSignature != BOOT_SIGNATURE)
	{
		CLogger::Get ()->Write (FromPartitionManager, LogWarning, "Drive has no MBR");

		return TRUE;
	}

	unsigned nPartition = 0;
	for (unsigned i = 0; i < MAX_PARTITIONS; i++)
	{
		if (   MBR.Partition[i].Type == 0
		    || MBR.Partition[i].Type == 0x05		// Extended partitions are not supported
		    || MBR.Partition[i].Type == 0x0F		// Extended partitions are not supported
		    || MBR.Partition[i].Type == 0xEF		// EFI is not supported
		    || MBR.Partition[i].LBAFirstSector == 0
		    || MBR.Partition[i].NumberOfSectors == 0)
		{
			continue;
		}

		assert (m_pPartition[i] == 0);
		m_pPartition[i] = new CPartition (m_pDevice, MBR.Partition[i].LBAFirstSector, 
						  MBR.Partition[i].NumberOfSectors);
		assert (m_pPartition[i] != 0);

		CString PartitionName;
		PartitionName.Format ("%s-%u", (const char *) m_DeviceName, ++nPartition);
		CDeviceNameService::Get ()->AddDevice (PartitionName, m_pPartition[i], TRUE);
	}

	if (nPartition == 0)
	{
		CLogger::Get ()->Write (FromPartitionManager, LogWarning, "Drive has no supported partition");

		return TRUE;
	}

	return TRUE;
}
