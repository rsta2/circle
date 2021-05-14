//
// fatfs.h
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
#ifndef _circle_fs_fat_fatfs_h
#define _circle_fs_fat_fatfs_h

#include <circle/fs/fsdef.h>
#include <circle/fs/fat/fatfsdef.h>
#include <circle/fs/fat/fatcache.h>
#include <circle/fs/fat/fatinfo.h>
#include <circle/fs/fat/fat.h>
#include <circle/fs/fat/fatdir.h>
#include <circle/device.h>
#include <circle/genericlock.h>
#include <circle/types.h>

struct TFile
{
	unsigned	 nUseCount;
	char		 chTitle[FS_TITLE_LEN+1];	/* real name, not the FAT name */
	unsigned	 nSize;
	unsigned	 nOffset;		/* current position */
	unsigned	 nCluster;		/* current cluster */
	unsigned	 nFirstCluster;		/* first cluster in chain, for write only */
	TFATBuffer	*pBuffer;		/* current buffer if available */
	boolean		 bWrite;		/* open for write */
};

#define FILE(handle)	m_Files[handle-1]

class CFATFileSystem
{
public:
	CFATFileSystem (void);
	~CFATFileSystem (void);

	/*
	 * Mount file system
	 * 
	 * Params:  pPartition		Partition to be used
	 * Returns: Nonzero on success
	 */
	int Mount (CDevice *pPartition);
	
	/*
	 * UnMount file system
	 * 
	 * Params:  none
	 * Returns: none
	 */
	void UnMount (void);

	/*
	 * Flush buffer cache
	 * 
	 * Params:  none
	 * Returns: none
	 */
	void Synchronize (void);

	/*
	* Find first directory entry
	*
	* Params:  pBuffer		Buffer to receive information
	*          pCurrentEntry	Pointer to current entry variable
	* Returns: != 0			Entry found
	*	    0			Not found
	*/
	unsigned RootFindFirst (TDirentry *pEntry, TFindCurrentEntry *pCurrentEntry);

	/*
	* Find next directory entry
	*
	* Params:  pBuffer		Buffer to receive information
	*          pCurrentEntry	Pointer to current entry variable
	* Returns: != 0			Entry found
	*	    0			No more entries
	*/
	unsigned RootFindNext (TDirentry *pEntry, TFindCurrentEntry *pCurrentEntry);

	/*
	* Open file for read
	*
	* Params:  pTitle	Title
	* Returns: != 0	File handle
	*	    0		Cannot open
	*/
	unsigned FileOpen (const char *pTitle);

	/*
	* Create new file for write (truncates file if it exists)
	*
	* Params:  pTitle	Title
	* Returns: != 0	File handle
	*	    0		Cannot create
	*/
	unsigned FileCreate (const char *pTitle);

	/*
	* Close file
	*
	* Params:  hFile	File handle
	* Returns: != 0	Success
	*	    0		Failure
	*/
	unsigned FileClose (unsigned hFile);

	/*
	* Read from file sequentially
	*
	* Params:  hFile	File handle
	*	    pBuffer	Buffer to copy data to
	*	    ulCount	Number of bytes to read
	* Returns: != 0	Number of bytes read
	*	    0		End of file
	*	    0xFFFFFFFF	General failure
	*/
	unsigned FileRead (unsigned hFile, void *pBuffer, unsigned nCount);

	/*
	* Write to file sequentially
	*
	* Params:  hFile	File handle
	*	    pBuffer	Buffer to copy data from
	*	    nCount	Number of bytes to write
	* Returns: != 0	Number of bytes written
	*	    0xFFFFFFFF	General failure
	*/
	unsigned FileWrite (unsigned hFile, const void *pBuffer, unsigned nCount);

	/*
	* Delete all root entries for title
	*
	* Params:  pTitle	Title
	* Returns: > 0		Success
	*	    0		Failure (title not found)
	* 	   < 0		Failure (read only file)
	*/
	int FileDelete (const char *pTitle);

private:
	CFATCache	m_Cache;
	CFATInfo	m_FATInfo;
	CFAT		m_FAT;
	CFATDirectory	m_Root;
	TFile		m_Files[FAT_FILES];

	CGenericLock m_FileTableLock;
};

#endif
