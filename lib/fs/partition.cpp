//
// partition.cpp
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
#include <circle/fs/partition.h>
#include <circle/fs/fsdef.h>
#include <assert.h>

CPartition::CPartition (CDevice *pDevice, unsigned nFirstSector, unsigned nNumberOfSectors)
:	m_pDevice (pDevice),
	m_nFirstSector (nFirstSector),
	m_nNumberOfSectors (nNumberOfSectors),
	m_ullOffset (0),
	m_bSeekError (TRUE)
{
	assert (m_pDevice != 0);
}

CPartition::~CPartition (void)
{
	m_pDevice = 0;
}

int CPartition::Read (void *pBuffer, size_t nCount)
{
	if (m_bSeekError)
	{
		return -1;
	}

	u64 ullTransferEnd = m_ullOffset + nCount + FS_BLOCK_SIZE-1;
	ullTransferEnd >>= FS_BLOCK_SHIFT;
	if (ullTransferEnd > m_nNumberOfSectors)
	{
		return -1;
	}
	
	assert (m_pDevice != 0);
	return m_pDevice->Read (pBuffer, nCount);
}

int CPartition::Write (const void *pBuffer, size_t nCount)
{
	if (m_bSeekError)
	{
		return -1;
	}

	u64 ullTransferEnd = m_ullOffset + nCount + FS_BLOCK_SIZE-1;
	ullTransferEnd >>= FS_BLOCK_SHIFT;
	if (ullTransferEnd > m_nNumberOfSectors)
	{
		return -1;
	}
	
	assert (m_pDevice != 0);
	return m_pDevice->Write (pBuffer, nCount);
}

u64 CPartition::Seek (u64 ullOffset)
{
	m_bSeekError = TRUE;

	if (   (ullOffset & FS_BLOCK_MASK) != 0
	    || (ullOffset >> FS_BLOCK_SHIFT) >= m_nNumberOfSectors)
	{
		return (u64) -1;
	}

	u64 ullDeviceOffset = m_nFirstSector;
	ullDeviceOffset <<= FS_BLOCK_SHIFT;
	ullDeviceOffset += ullOffset;
	
	assert (m_pDevice != 0);
	if (m_pDevice->Seek (ullDeviceOffset) != ullDeviceOffset)
	{
		return (u64) -1;
	}

	m_ullOffset = ullOffset;
	m_bSeekError = FALSE;

	return m_ullOffset;
}
