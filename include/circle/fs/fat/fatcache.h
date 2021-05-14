//
// fatcache.h
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
#ifndef _circle_fs_fat_fatcache_h
#define _circle_fs_fat_fatcache_h

#include <circle/fs/fat/fatfsdef.h>
#include <circle/device.h>
#include <circle/genericlock.h>
#include <circle/synchronize.h>

struct TFATBuffer
{
	unsigned	 nMagic;
	TFATBuffer	*pNext;
	TFATBuffer	*pPrev;
	unsigned	 nSector;
	unsigned	 nUseCount;
	int		 bDirty;

	DMA_BUFFER (unsigned char, Data, FAT_SECTOR_SIZE);
};

struct TFATBufferList
{
	TFATBuffer *pFirst;
	TFATBuffer *pLast;
};

class CFATCache
{
public:
	CFATCache (void);
	~CFATCache (void);

	/*
	 * Open buffer cache
	 *
	 * Params:  pPartition		Partition to be used
	 * Returns: Nonzero on success
	 */
	int Open (CDevice *pPartition);
	
	/*
	 * Close buffer cache
	 *
	 * Params:  none
	 * Returns: none
	 */
	void Close (void);
	
	/*
	 * Flush buffer cache
	 *
	 * Params:  none
	 * Returns: none
	 */
	void Flush (void);
	
	/*
	 * Get sector from buffer cache
	 *
	 * Params:  nSector	Sector number
	 *	    bWriteOnly	Do not read physical block into buffer
	 * Returns: != 0	Pointer to buffer
	 *	    0		Failure
	 */
	TFATBuffer *GetSector (unsigned nSector, int bWriteOnly);
	
	/*
	 * Free sector in buffer cache
	 *
	 * Params:  pBuffer	Pointer to buffer
	 *	    bCritical	Sector is part of critical data
	 * Returns: none
	 */
	void FreeSector (TFATBuffer *pBuffer, int bCritical);
	
	/*
	 * Mark buffer dirty (has to be written to disk)
	 *
	 * Params:  pBuffer	Pointer to buffer
	 * Returns: none
	 */
	void MarkDirty (TFATBuffer *pBuffer);

private:
	void MoveBufferFirst (TFATBuffer *pBuffer);
	void MoveBufferLast (TFATBuffer *pBuffer);

	void Fault (unsigned nCode);

private:
	CDevice		*m_pPartition;
	TFATBufferList	 m_BufferList;

	CGenericLock m_BufferListLock;
	CGenericLock m_DiskLock;
};

#endif
