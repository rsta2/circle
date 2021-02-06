//
// fatinfo.h
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
#ifndef _circle_fs_fat_fatinfo_h
#define _circle_fs_fat_fatinfo_h

#include <circle/fs/fat/fatcache.h>
#include <circle/genericlock.h>
#include <circle/types.h>

enum TFATType
{
	//FAT12,	// not supported
	FAT16,
	FAT32,
	FATUnknown
};

class CFATInfo
{
public:
	CFATInfo (CFATCache *pCache);
	~CFATInfo (void);

	boolean Initialize (void);

	// Boot sector
	TFATType GetFATType (void) const;

	unsigned GetSectorsPerCluster (void) const;
	unsigned GetReservedSectors (void) const;
	unsigned GetClusterCount (void) const;

	unsigned GetReadFAT (void) const;
	unsigned GetFirstWriteFAT (void) const;
	unsigned GetLastWriteFAT (void) const;
	unsigned GetFATSize (void) const;

	unsigned GetFirstRootSector (void) const;	// FAT16 only
	unsigned GetRootSectorCount (void) const;	// FAT16 only
	unsigned GetRootEntries (void) const;		// FAT16 only
	unsigned GetRootCluster (void) const;		// FAT32 only

	unsigned GetFirstSector (unsigned nCluster) const;

	// FS Info sector
	void UpdateFSInfo (void);

	void ClusterAllocated (unsigned nCluster);
	void ClusterFreed (unsigned nCluster);
	
	unsigned GetNextFreeCluster (void);

private:
	CFATCache *m_pCache;

	TFATType m_FATType;

	// all FAT types
	unsigned m_nSectorsPerCluster;
	unsigned m_nReservedSectors;
	unsigned m_nNumberOfFATs;
	unsigned m_nRootEntries;
	unsigned m_nFATSize;
	unsigned m_nTotalSectors;
	boolean  m_bFATMirroring;

	// FAT32 only
	unsigned m_nActiveFAT;
	unsigned m_nRootCluster;
	unsigned m_nFSInfoSector;

	// calculated
	unsigned m_nRootSectors;
	unsigned m_nFirstDataSector;
	unsigned m_nDataSectors;
	unsigned m_nClusters;

	// from FS Info sector (saved on FAT32 only)
	unsigned m_nFreeCount;
	unsigned m_nNextFreeCluster;

	CGenericLock m_Lock;
};

#endif
