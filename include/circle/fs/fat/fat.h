//
// fat.h
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
#ifndef _circle_fs_fat_fat_h
#define _circle_fs_fat_fat_h

#include <circle/fs/fat/fatcache.h>
#include <circle/fs/fat/fatinfo.h>
#include <circle/genericlock.h>
#include <circle/types.h>

class CFAT
{
public:
	CFAT (CFATCache *pCache, CFATInfo *pFATInfo);
	~CFAT (void);

	unsigned GetClusterEntry (unsigned nCluster);
	boolean IsEOC (unsigned nClusterEntry) const;		// end of cluster chain?
	void SetClusterEntry (unsigned nCluster, unsigned nEntry);

	unsigned AllocateCluster (void);			// returns 0 on failure
	void FreeClusterChain (unsigned nFirstCluster);

private:
	TFATBuffer *GetSector (unsigned nCluster, unsigned *pSectorOffset, unsigned nFAT);

	unsigned GetEntry (TFATBuffer *pBuffer, unsigned nSectorOffset);
	void SetEntry (TFATBuffer *pBuffer, unsigned nSectorOffset, unsigned nEntry);

private:
	CFATCache *m_pCache;
	CFATInfo  *m_pFATInfo;

	CGenericLock m_Lock;
};

#endif
