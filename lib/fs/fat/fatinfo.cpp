//
// fatinfo.cpp
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
#include <circle/fs/fat/fatinfo.h>
#include <circle/fs/fat/fatfsdef.h>
#include <circle/logger.h>
#include <assert.h>

static const char FromFATInfo[] = "fatinfo";

CFATInfo::CFATInfo (CFATCache *pCache)
:	m_pCache (pCache),
	m_FATType (FATUnknown)
{
}

CFATInfo::~CFATInfo (void)
{
	m_FATType = FATUnknown;
	m_pCache = 0;
}

boolean CFATInfo::Initialize (void)
{
	assert (m_pCache != 0);
	TFATBuffer *pBuffer = m_pCache->GetSector (0, 0);
	assert (pBuffer != 0);

	TFATBootSector *pBoot = (TFATBootSector *) pBuffer->Data;
	assert (pBoot != 0);

	if (    pBoot->nBootSignature      != BOOT_SIGNATURE
	    ||	pBoot->BPB.nBytesPerSector != FAT_SECTOR_SIZE
	    || (pBoot->BPB.nMedia & 0xF0)  != 0xF0)
	{
		m_pCache->FreeSector (pBuffer, 0);
		return FALSE;
	}

	m_nSectorsPerCluster = pBoot->BPB.nSectorsPerCluster;
	m_nReservedSectors   = pBoot->BPB.nReservedSectors;
	m_nNumberOfFATs      = pBoot->BPB.nNumberOfFATs;
	m_nRootEntries       = pBoot->BPB.nRootEntries;

	m_nFATSize = pBoot->BPB.nFATSize16;
	if (m_nFATSize != 0)
	{
		m_FATType = FAT16;

		m_bFATMirroring = TRUE;
	}
	else
	{
		m_FATType = FAT32;

		if (   pBoot->Struct.FAT32.nBootSignature != 0x29
		    || pBoot->Struct.FAT32.nFSVersion     != 0
		    || m_nRootEntries                     != 0)
		{
			m_pCache->FreeSector (pBuffer, 0);
			return FALSE;
		}

		m_nFATSize = pBoot->Struct.FAT32.nFATSize32;

		m_bFATMirroring = !pBoot->Struct.FAT32.nMirroringOff;
		if (m_bFATMirroring)
		{
			m_nActiveFAT = pBoot->Struct.FAT32.nActiveFAT;

			if (m_nActiveFAT >= m_nNumberOfFATs)
			{
				m_pCache->FreeSector (pBuffer, 0);
				return FALSE;
			}
		}

		m_nRootCluster  = pBoot->Struct.FAT32.nRootCluster;
		m_nFSInfoSector = pBoot->Struct.FAT32.nFSInfoSector;
	}

	m_nTotalSectors = pBoot->BPB.nTotalSectors16;
	if (m_nTotalSectors == 0)
	{
		m_nTotalSectors = pBoot->BPB.nTotalSectors32;
	}

	pBoot = 0;
	m_pCache->FreeSector (pBuffer, 0);
	pBuffer = 0;

	if (   m_nReservedSectors == 0
	    || m_nFATSize == 0)
	{
		return FALSE;
	}

	m_nRootSectors     = ((m_nRootEntries * 32) + (FAT_SECTOR_SIZE - 1)) / FAT_SECTOR_SIZE;
	m_nFirstDataSector = m_nReservedSectors + (m_nNumberOfFATs * m_nFATSize) + m_nRootSectors;
	m_nDataSectors     = m_nTotalSectors - m_nFirstDataSector;
	m_nClusters        = m_nDataSectors / m_nSectorsPerCluster;

	if (m_nClusters < 4085)
	{
		return FALSE;			// FAT12 is not supported
	}
	else if (m_nClusters < 65525)
	{
		if (m_FATType != FAT16)
		{
			return FALSE;
		}
	}
	else
	{
		if (m_FATType != FAT32)
		{
			return FALSE;
		}
	}

	if (m_FATType == FAT32)
	{
		if (   m_nFSInfoSector == 0
		    || m_nFSInfoSector >= m_nReservedSectors)
		{
			return FALSE;
		}

		assert (pBuffer == 0);
		pBuffer = m_pCache->GetSector (m_nFSInfoSector, 0);
		assert (pBuffer != 0);

		TFAT32FSInfoSector *pFSInfo = (TFAT32FSInfoSector *) pBuffer->Data;
		assert (pFSInfo != 0);

		if (   pFSInfo->nLeadingSignature  != LEADING_SIGNATURE
		    || pFSInfo->nStructSignature   != STRUCT_SIGNATURE
		    || pFSInfo->nTrailingSignature != TRAILING_SIGNATURE)
		{
			m_pCache->FreeSector (pBuffer, 0);
			return FALSE;
		}

		m_nFreeCount       = pFSInfo->nFreeCount;
		m_nNextFreeCluster = pFSInfo->nNextFreeCluster;

		pFSInfo = 0;
		m_pCache->FreeSector (pBuffer, 0);
		pBuffer = 0;

		if (m_nNextFreeCluster == NEXT_FREE_CLUSTER_UNKNOWN)
		{
			m_nNextFreeCluster = 2;
		}
	}
	else
	{
		m_nFreeCount       = FREE_COUNT_UNKNOWN;
		m_nNextFreeCluster = 2;
	}

	// Try to read last data sector
	assert (pBuffer == 0);
	pBuffer = m_pCache->GetSector (m_nFirstDataSector + m_nDataSectors - 1, 0);
	assert (pBuffer != 0);
	m_pCache->FreeSector (pBuffer, 0);
	pBuffer = 0;

	unsigned nClusterSize = 10 * m_nSectorsPerCluster * FAT_SECTOR_SIZE / 1024;
	CLogger::Get ()->Write (FromFATInfo, LogDebug, "FAT%u: %u clusters of %u.%uK",
				m_FATType == FAT16 ? 16 : 32, m_nClusters,
				nClusterSize / 10, nClusterSize % 10);

	return TRUE;
}

TFATType CFATInfo::GetFATType (void) const
{
	assert (m_FATType != FATUnknown);
	return m_FATType;
}

unsigned CFATInfo::GetSectorsPerCluster (void) const
{
	return m_nSectorsPerCluster;
}

unsigned CFATInfo::GetReservedSectors (void) const
{
	return m_nReservedSectors;
}

unsigned CFATInfo::GetClusterCount (void) const
{
	return m_nClusters;
}

unsigned CFATInfo::GetReadFAT (void) const
{
	if (   m_FATType == FAT32
	    && !m_bFATMirroring)
	{
		return m_nActiveFAT;
	}

	return 0;
}

unsigned CFATInfo::GetFirstWriteFAT (void) const
{
	if (   m_FATType == FAT32
	    && !m_bFATMirroring)
	{
		return m_nActiveFAT;
	}

	return 0;
}

unsigned CFATInfo::GetLastWriteFAT (void) const
{
	if (   m_FATType == FAT32
	    && !m_bFATMirroring)
	{
		return m_nActiveFAT;
	}

	assert (m_nNumberOfFATs > 0);
	return m_nNumberOfFATs-1;
}

unsigned CFATInfo::GetFATSize (void) const
{
	return m_nFATSize;
}

unsigned CFATInfo::GetFirstRootSector (void) const
{
	assert (m_FATType == FAT16);
	return m_nReservedSectors + (m_nNumberOfFATs * m_nFATSize);
}

unsigned CFATInfo::GetRootSectorCount (void) const
{
	assert (m_FATType == FAT16);
	return m_nRootSectors;
}

unsigned CFATInfo::GetRootEntries (void) const
{
	assert (m_FATType == FAT16);
	return m_nRootEntries;
}

unsigned CFATInfo::GetRootCluster (void) const
{
	assert (m_FATType == FAT32);
	return m_nRootCluster;
}

unsigned CFATInfo::GetFirstSector (unsigned nCluster) const
{
	nCluster -= 2;
	assert (nCluster < m_nClusters);

	return m_nFirstDataSector + (nCluster * m_nSectorsPerCluster);
}

void CFATInfo::UpdateFSInfo (void)
{
	if (m_FATType != FAT32)
	{
		return;
	}

	m_Lock.Acquire ();
	
	assert (m_pCache != 0);
	TFATBuffer *pBuffer = m_pCache->GetSector (m_nFSInfoSector, 0);
	assert (pBuffer != 0);

	TFAT32FSInfoSector *pFSInfo = (TFAT32FSInfoSector *) pBuffer->Data;
	assert (pFSInfo != 0);

	if (   pFSInfo->nLeadingSignature  != LEADING_SIGNATURE
	    || pFSInfo->nStructSignature   != STRUCT_SIGNATURE
	    || pFSInfo->nTrailingSignature != TRAILING_SIGNATURE)
	{
		m_pCache->FreeSector (pBuffer, 1);
		m_Lock.Release ();
		return;
	}

	pFSInfo->nFreeCount       = m_nFreeCount;
	pFSInfo->nNextFreeCluster = m_nNextFreeCluster;

	m_pCache->MarkDirty (pBuffer);

	pFSInfo = 0;
	m_pCache->FreeSector (pBuffer, 1);
	pBuffer = 0;

	m_Lock.Release ();
}

void CFATInfo::ClusterAllocated (unsigned nCluster)
{
	m_Lock.Acquire ();

	if (   m_nFreeCount != FREE_COUNT_UNKNOWN
	    && m_nFreeCount > 0)
	{
		m_nFreeCount--;
	}

	if (nCluster <= m_nClusters)
	{
		m_nNextFreeCluster = nCluster+1;
	}
	else
	{
		m_nNextFreeCluster = 2;
	}
	
	m_Lock.Release ();
}

void CFATInfo::ClusterFreed (unsigned nCluster)
{
	m_Lock.Acquire ();

	if (   m_nFreeCount != FREE_COUNT_UNKNOWN
	    && m_nFreeCount < m_nClusters)
	{
		m_nFreeCount++;
	}

	assert (m_nNextFreeCluster != NEXT_FREE_CLUSTER_UNKNOWN);
	assert (m_nNextFreeCluster >= 2);
	assert (m_nNextFreeCluster <= m_nClusters+1);
	if (nCluster < m_nNextFreeCluster)
	{
		m_nNextFreeCluster = nCluster;
	}

	m_Lock.Release ();
}

unsigned CFATInfo::GetNextFreeCluster (void)
{
	m_Lock.Acquire ();

	unsigned nCluster = m_nNextFreeCluster;

	m_Lock.Release ();

	return nCluster;
}
