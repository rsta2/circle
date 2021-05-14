//
// fatdir.h
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
#ifndef _circle_fs_fat_fatdir_h
#define _circle_fs_fat_fatdir_h

#include <circle/fs/fsdef.h>
#include <circle/fs/fat/fatcache.h>
#include <circle/fs/fat/fatinfo.h>
#include <circle/fs/fat/fat.h>
#include <circle/genericlock.h>
#include <circle/types.h>
 
class CFATDirectory
{
public:
	CFATDirectory (CFATCache *pCache, CFATInfo *pFATInfo, CFAT *pFAT);
	~CFATDirectory (void);

	TFATDirectoryEntry *GetEntry (const char *pName);
	TFATDirectoryEntry *CreateEntry (const char *pName);
	void FreeEntry (boolean bChanged);

	boolean FindFirst (TDirentry *pEntry, TFindCurrentEntry *pCurrentEntry);
	boolean FindNext (TDirentry *pEntry, TFindCurrentEntry *pCurrentEntry);

	static unsigned Time2FAT (unsigned nTime);	// returns FAT time (date << 16 | time)

private:
	static boolean Name2FAT (const char *pName, char *pFATName);
	static void FAT2Name (const char *pFATName, char *pName);

private:
	CFATCache *m_pCache;
	CFATInfo  *m_pFATInfo;
	CFAT	  *m_pFAT;

	TFATBuffer *m_pBuffer;

	CGenericLock m_Lock;
};

#endif
