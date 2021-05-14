//
// fat.cpp
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
#include <circle/fs/fat/fat.h>
#include <assert.h>

CFAT::CFAT (CFATCache *pCache, CFATInfo *pFATInfo)
:	m_pCache (pCache),
	m_pFATInfo (pFATInfo)
{
}

CFAT::~CFAT (void)
{
	m_pCache = 0;
	m_pFATInfo = 0;
}

unsigned CFAT::GetClusterEntry (unsigned nCluster)
{
	m_Lock.Acquire ();

	unsigned nSectorOffset;

	assert (m_pFATInfo != 0);
	TFATBuffer *pBuffer = GetSector (nCluster, &nSectorOffset, m_pFATInfo->GetReadFAT ());
	assert (pBuffer != 0);

	unsigned nEntry = GetEntry (pBuffer, nSectorOffset);

	assert (m_pCache != 0);
	m_pCache->FreeSector (pBuffer, 1);

	m_Lock.Release ();
	
	return nEntry;
}

boolean CFAT::IsEOC (unsigned nClusterEntry) const
{
	assert (m_pFATInfo != 0);
	if (m_pFATInfo->GetFATType () == FAT16)
	{
		if (nClusterEntry >= 0xFFF8)
		{
			return TRUE;
		}
	}
	else
	{
		assert (m_pFATInfo->GetFATType () == FAT32);

		if (nClusterEntry >= 0x0FFFFFF8)
		{
			return TRUE;
		}
	}

	return FALSE;
}

void CFAT::SetClusterEntry (unsigned nCluster, unsigned nEntry)
{
	m_Lock.Acquire ();

	assert (m_pFATInfo != 0);
	for (unsigned nFAT = m_pFATInfo->GetFirstWriteFAT (); nFAT <= m_pFATInfo->GetLastWriteFAT (); nFAT++)
	{
		unsigned nSectorOffset;
		TFATBuffer *pBuffer = GetSector (nCluster, &nSectorOffset, nFAT);
		assert (pBuffer != 0);

		SetEntry (pBuffer, nSectorOffset, nEntry);

		assert (m_pCache != 0);
		m_pCache->MarkDirty (pBuffer);
		m_pCache->FreeSector (pBuffer, 1);
	}

	m_Lock.Release ();
}

unsigned CFAT::AllocateCluster (void)
{
	m_Lock.Acquire ();

	assert (m_pFATInfo != 0);
	unsigned nCluster = m_pFATInfo->GetNextFreeCluster ();

	while (nCluster < m_pFATInfo->GetClusterCount () + 2)
	{
		assert (nCluster >= 2);

		unsigned nSectorOffset;
		TFATBuffer *pBuffer = GetSector (nCluster, &nSectorOffset, m_pFATInfo->GetReadFAT ());
		assert (pBuffer != 0);

		unsigned nClusterEntry = GetEntry (pBuffer, nSectorOffset);
		
		assert (m_pCache != 0);
		m_pCache->FreeSector (pBuffer, 1);
		
		if (nClusterEntry == 0)
		{
			for (unsigned nFAT = m_pFATInfo->GetFirstWriteFAT ();
			     nFAT <= m_pFATInfo->GetLastWriteFAT (); nFAT++)
			{
				pBuffer = GetSector (nCluster, &nSectorOffset, nFAT);
				assert (pBuffer != 0);

				SetEntry (pBuffer, nSectorOffset, m_pFATInfo->GetFATType () == FAT16 ? 0xFFFF : 0x0FFFFFFF);
				
				m_pCache->MarkDirty (pBuffer);
				m_pCache->FreeSector (pBuffer, 1);
			}

			m_pFATInfo->ClusterAllocated (nCluster);

			m_Lock.Release ();

			return nCluster;
		}

		nCluster++;
	}

	m_Lock.Release ();

	return 0;
}

void CFAT::FreeClusterChain (unsigned nFirstCluster)
{
	do
	{
		unsigned nNextCluster = GetClusterEntry (nFirstCluster);

		SetClusterEntry (nFirstCluster, 0);

		assert (m_pFATInfo != 0);
		m_pFATInfo->ClusterFreed (nFirstCluster);

		nFirstCluster = nNextCluster;
	}
	while (!IsEOC (nFirstCluster));
}

TFATBuffer *CFAT::GetSector (unsigned nCluster, unsigned *pSectorOffset, unsigned nFAT)
{
	assert (nCluster >= 2);
	
	unsigned nFATOffset;

	assert (m_pFATInfo != 0);
	if (m_pFATInfo->GetFATType () == FAT16)
	{
		nFATOffset = nCluster * 2;
	}
	else
	{
		assert (m_pFATInfo->GetFATType () == FAT32);

		nFATOffset = nCluster * 4;
	}

	unsigned nFATSector =   m_pFATInfo->GetReservedSectors ()
			      + nFAT * m_pFATInfo->GetFATSize ()
			      + (nFATOffset / FAT_SECTOR_SIZE);

	assert (pSectorOffset != 0);
	*pSectorOffset = nFATOffset % FAT_SECTOR_SIZE;

	assert (m_pCache != 0);
	return m_pCache->GetSector (nFATSector, 0);
}

unsigned CFAT::GetEntry (TFATBuffer *pBuffer, unsigned nSectorOffset)
{
	assert (pBuffer != 0);

	unsigned nEntry;
	if (m_pFATInfo->GetFATType () == FAT16)
	{
		assert (nSectorOffset <= FAT_SECTOR_SIZE-2);
		nEntry =   (u16) pBuffer->Data[nSectorOffset]
			 | (u16) pBuffer->Data[nSectorOffset+1] << 8;
	}
	else
	{
		assert (nSectorOffset <= FAT_SECTOR_SIZE-4);
		nEntry = (  (u32) pBuffer->Data[nSectorOffset]
			  | (u32) pBuffer->Data[nSectorOffset+1] << 8
			  | (u32) pBuffer->Data[nSectorOffset+2] << 16
			  | (u32) pBuffer->Data[nSectorOffset+3] << 24) & 0x0FFFFFFF;
	}

	return nEntry;
}

void CFAT::SetEntry (TFATBuffer *pBuffer, unsigned nSectorOffset, unsigned nEntry)
{
	assert (pBuffer != 0);

	if (m_pFATInfo->GetFATType () == FAT16)
	{
		assert (nSectorOffset <= FAT_SECTOR_SIZE-2);
		assert (nEntry <= 0xFFFF);
		pBuffer->Data[nSectorOffset]   = (u8) (nEntry & 0xFF);
		pBuffer->Data[nSectorOffset+1] = (u8) (nEntry >> 8);
	}
	else
	{
		assert (nSectorOffset <= FAT_SECTOR_SIZE-4);
		assert (nEntry <= 0x0FFFFFFF);
		pBuffer->Data[nSectorOffset]   = (u8) (nEntry & 0xFF);
		pBuffer->Data[nSectorOffset+1] = (u8) (nEntry >> 8  & 0xFF);
		pBuffer->Data[nSectorOffset+2] = (u8) (nEntry >> 16 & 0xFF);
		pBuffer->Data[nSectorOffset+3] &= 0xF0;
		pBuffer->Data[nSectorOffset+3] |= (u8) (nEntry >> 24 & 0x0F);
	}
}
