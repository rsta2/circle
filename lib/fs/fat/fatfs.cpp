//
// fatfs.cpp
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
#include <circle/fs/fat/fatfs.h>
#include <circle/timer.h>
#include <circle/util.h>
#include <assert.h>

CFATFileSystem::CFATFileSystem (void)
:	m_FATInfo (&m_Cache),
	m_FAT (&m_Cache, &m_FATInfo),
	m_Root (&m_Cache, &m_FATInfo, &m_FAT)
{
	memset (&m_Files, 0, sizeof m_Files);
}

CFATFileSystem::~CFATFileSystem (void)
{
}

int CFATFileSystem::Mount (CDevice *pPartition)
{
	if (!m_Cache.Open (pPartition))
	{
		return 0;
	}

	if (!m_FATInfo.Initialize ())
	{
		m_Cache.Close ();
		return 0;
	}

	return 1;
}

void CFATFileSystem::UnMount (void)
{
	m_FATInfo.UpdateFSInfo ();

	m_Cache.Close ();
}

void CFATFileSystem::Synchronize (void)
{
	m_FATInfo.UpdateFSInfo ();

	m_Cache.Flush ();
}

unsigned CFATFileSystem::RootFindFirst (TDirentry *pEntry, TFindCurrentEntry *pCurrentEntry)
{
	return m_Root.FindFirst (pEntry, pCurrentEntry) ? 1 : 0;
}

unsigned CFATFileSystem::RootFindNext (TDirentry *pEntry, TFindCurrentEntry *pCurrentEntry)
{
	return m_Root.FindNext (pEntry, pCurrentEntry) ? 1 : 0;
}

unsigned CFATFileSystem::FileOpen (const char *pTitle)
{
	m_FileTableLock.Acquire ();

	unsigned hFile;
	for (hFile = 1; hFile <= FAT_FILES; hFile++)
	{
		if (!FILE (hFile).nUseCount)
		{
			break;
		}
	}

	if (hFile > FAT_FILES)
	{
		m_FileTableLock.Release ();

		return 0;
	}

	assert (pTitle != 0);
	TFATDirectoryEntry *pEntry = m_Root.GetEntry (pTitle);
	if (pEntry == 0)
	{
		m_FileTableLock.Release ();

		return 0;
	}

	TFile *pFile = &FILE (hFile);

	pFile->nUseCount = 1;
	strncpy (pFile->chTitle, pTitle, FS_TITLE_LEN);
	pFile->chTitle[FS_TITLE_LEN] = '\0';
	pFile->nSize = pEntry->nFileSize;
	pFile->nOffset = 0;
	pFile->nCluster = (unsigned) pEntry->nFirstClusterHigh << 16 | pEntry->nFirstClusterLow;
	pFile->nFirstCluster = 0;
	pFile->pBuffer = 0;
	pFile->bWrite = FALSE;

	m_Root.FreeEntry (FALSE);

	m_FileTableLock.Release ();

	return hFile;
}

unsigned CFATFileSystem::FileCreate (const char *pTitle)
{
	m_FileTableLock.Acquire ();

	unsigned hFile;
	for (hFile = 1; hFile <= FAT_FILES; hFile++)
	{
		if (!FILE (hFile).nUseCount)
		{
			break;
		}
	}

	if (hFile > FAT_FILES)
	{
		m_FileTableLock.Release ();

		return 0;
	}

	assert (pTitle != 0);
	if (FileDelete (pTitle) < 0)
	{
		m_FileTableLock.Release ();

		return 0;
	}

	TFATDirectoryEntry *pEntry = m_Root.CreateEntry (pTitle);
	if (pEntry == 0)
	{
		m_FileTableLock.Release ();

		return 0;
	}

	m_Root.FreeEntry (TRUE);

	TFile *pFile = &FILE (hFile);

	pFile->nUseCount = 1;
	strncpy (pFile->chTitle, pTitle, FS_TITLE_LEN);
	pFile->chTitle[FS_TITLE_LEN] = '\0';
	pFile->nSize = 0;
	pFile->nOffset = 0;
	pFile->nCluster = 0;
	pFile->nFirstCluster = 0;
	pFile->pBuffer = 0;
	pFile->bWrite = 1;

	m_FileTableLock.Release ();

	return hFile;
}

unsigned CFATFileSystem::FileClose (unsigned hFile)
{
	if (!(   1 <= hFile
	      && hFile <= FAT_FILES))
	{
		return 0;
	}

	m_FileTableLock.Acquire ();

	TFile *pFile = &FILE (hFile);
	if (!pFile->nUseCount)
	{
		m_FileTableLock.Release ();
		return 0;
	}

	if (pFile->nUseCount > 1)
	{
		pFile->nUseCount--;
		m_FileTableLock.Release ();
		return 1;
	}

	if (pFile->pBuffer != 0)
	{
		m_Cache.FreeSector (pFile->pBuffer, 0);
		pFile->pBuffer = 0;
	}

	if (pFile->bWrite)
	{
		TFATDirectoryEntry *pEntry = m_Root.GetEntry (pFile->chTitle);
		if (pEntry != 0)
		{
			pEntry->nAttributes |= FAT_DIR_ATTR_ARCHIVE;

			pEntry->nFirstClusterHigh = pFile->nFirstCluster >> 16;
			pEntry->nFirstClusterLow  = pFile->nFirstCluster & 0xFFFF;

			pEntry->nFileSize = pFile->nSize;

			unsigned nDateTime = m_Root.Time2FAT (CTimer::Get ()->GetLocalTime ());
			pEntry->nWriteDate = nDateTime >> 16;
			pEntry->nWriteTime = nDateTime & 0xFFFF;

			m_Root.FreeEntry (TRUE);
		}

		Synchronize ();
	}

	pFile->nUseCount = 0;

	m_FileTableLock.Release ();

	return 1;
}

unsigned CFATFileSystem::FileRead (unsigned hFile, void *pBuffer, unsigned ulBytes)
{
	unsigned ulBytesRead = 0;
	TFile *pFile;

	if (!(   1 <= hFile
	      && hFile <= FAT_FILES))
	{
		return FS_ERROR;
	}

	m_FileTableLock.Acquire ();

	pFile = &FILE (hFile);
	if (   !pFile->nUseCount
	    || pFile->bWrite
	    || ulBytes == 0)
	{
		m_FileTableLock.Release ();
		return FS_ERROR;
	}

	while (ulBytes > 0)
	{
		unsigned ulBlockOffset;
		unsigned ulBytesLeft;
		unsigned ulCopyBytes;

		assert (pFile->nSize >= pFile->nOffset);
		ulBytesLeft = pFile->nSize - pFile->nOffset;
		if (ulBytesLeft == 0)
		{
			m_FileTableLock.Release ();
			return ulBytesRead;
		}
	
		if (pFile->pBuffer == 0)
		{
			unsigned nSectorOffset = pFile->nOffset / FAT_SECTOR_SIZE;
			unsigned nClusterOffset = nSectorOffset % m_FATInfo.GetSectorsPerCluster ();
			if (nClusterOffset == 0)
			{
				if (pFile->nOffset > 0)
				{
					pFile->nCluster = m_FAT.GetClusterEntry (pFile->nCluster);
					if (m_FAT.IsEOC (pFile->nCluster))
					{
						m_FileTableLock.Release ();
						return FS_ERROR;
					}
				}
			}

			unsigned nSector = m_FATInfo.GetFirstSector (pFile->nCluster) + nClusterOffset;

			pFile->pBuffer = m_Cache.GetSector (nSector, 0);
			assert (pFile->pBuffer != 0);
		}
	
		ulCopyBytes = ulBytesLeft;
		if (ulBytesLeft > FAT_SECTOR_SIZE)
		{
			ulCopyBytes = FAT_SECTOR_SIZE;
		}

		if (ulCopyBytes > ulBytes)
		{
			ulCopyBytes = ulBytes;
		}

		ulBlockOffset = pFile->nOffset % FAT_SECTOR_SIZE;
		if (ulCopyBytes > FAT_SECTOR_SIZE - ulBlockOffset)
		{
			ulCopyBytes = FAT_SECTOR_SIZE - ulBlockOffset;
		}

		assert (pBuffer != 0);
		assert (ulCopyBytes > 0);
		memcpy (pBuffer, &pFile->pBuffer->Data[ulBlockOffset], ulCopyBytes);

		pBuffer = (void *) (((unsigned char *) pBuffer) + ulCopyBytes);

		pFile->nOffset += ulCopyBytes;

		ulBytes -= ulCopyBytes;
		ulBytesRead += ulCopyBytes;

		if ((pFile->nOffset % FAT_SECTOR_SIZE) == 0)
		{
			assert (pFile->pBuffer != 0);
			m_Cache.FreeSector (pFile->pBuffer, 0);
			pFile->pBuffer = 0;
		}
	}

	m_FileTableLock.Release ();

	return ulBytesRead;
}

unsigned CFATFileSystem::FileWrite (unsigned hFile, const void *pBuffer, unsigned ulBytes)
{
	unsigned int ulBytesWritten = 0;
	TFile *pFile;

	if (!(   1 <= hFile
	      && hFile <= FAT_FILES))
	{
		return FS_ERROR;
	}

	m_FileTableLock.Acquire ();

	pFile = &FILE (hFile);
	if (   !pFile->nUseCount
	    || !pFile->bWrite
	    || ulBytes == 0)
	{
		m_FileTableLock.Release ();
		return FS_ERROR;
	}

	while (ulBytes > 0)
	{
		unsigned ulBlockOffset;
		unsigned ulBytesLeft;
		unsigned ulCopyBytes;

		assert (pFile->nSize <= FAT_MAX_FILESIZE);
		ulBytesLeft = FAT_MAX_FILESIZE - pFile->nSize;
		if (ulBytesLeft == 0)
		{
			m_FileTableLock.Release ();
			return ulBytesWritten;
		}
	
		if (pFile->pBuffer == 0)
		{
			unsigned nSectorOffset = pFile->nOffset / FAT_SECTOR_SIZE;
			unsigned nClusterOffset = nSectorOffset % m_FATInfo.GetSectorsPerCluster ();
			if (nClusterOffset == 0)
			{
				unsigned nNextCluster = m_FAT.AllocateCluster ();
				if (nNextCluster == 0)
				{
					m_FileTableLock.Release ();
					return FS_ERROR;
				}
				
				if (pFile->nFirstCluster == 0)
				{
					pFile->nFirstCluster = nNextCluster;
				}
				else
				{
					m_FAT.SetClusterEntry (pFile->nCluster, nNextCluster);
				}

				pFile->nCluster = nNextCluster;
			}

			unsigned nSector = m_FATInfo.GetFirstSector (pFile->nCluster) + nClusterOffset;

			pFile->pBuffer = m_Cache.GetSector (nSector, 1);
			assert (pFile->pBuffer != 0);
		}
	
		ulCopyBytes = ulBytesLeft;
		if (ulBytesLeft > FAT_SECTOR_SIZE)
		{
			ulCopyBytes = FAT_SECTOR_SIZE;
		}

		if (ulCopyBytes > ulBytes)
		{
			ulCopyBytes = ulBytes;
		}

		ulBlockOffset = pFile->nOffset % FAT_SECTOR_SIZE;
		if (ulCopyBytes > FAT_SECTOR_SIZE - ulBlockOffset)
		{
			ulCopyBytes = FAT_SECTOR_SIZE - ulBlockOffset;
		}

		assert (pBuffer != 0);
		assert (ulCopyBytes > 0);
		memcpy (&pFile->pBuffer->Data[ulBlockOffset], pBuffer, ulCopyBytes);

		pBuffer = (void *) (((unsigned char *) pBuffer) + ulCopyBytes);

		m_Cache.MarkDirty (pFile->pBuffer);

		pFile->nOffset += ulCopyBytes;
		assert (pFile->nOffset < FAT_MAX_FILESIZE);
		pFile->nSize += ulCopyBytes;
		assert (pFile->nSize == pFile->nOffset);

		ulBytes -= ulCopyBytes;
		ulBytesWritten += ulCopyBytes;

		if ((pFile->nOffset % FAT_SECTOR_SIZE) == 0)
		{
			assert (pFile->pBuffer != 0);
			m_Cache.FreeSector (pFile->pBuffer, 0);
			pFile->pBuffer = 0;
		}
	}

	m_FileTableLock.Release ();

	return ulBytesWritten;
}

int CFATFileSystem::FileDelete (const char *pTitle)
{
	assert (pTitle != 0);
	TFATDirectoryEntry *pEntry = m_Root.GetEntry (pTitle);
	if (pEntry == 0)
	{
		return 0;
	}

	if (pEntry->nAttributes & FAT_DIR_ATTR_READ_ONLY)
	{
		m_Root.FreeEntry (TRUE);
		return -1;
	}
	
	unsigned nFirstCluster =   (unsigned) pEntry->nFirstClusterHigh << 16
				 | pEntry->nFirstClusterLow;
	if (nFirstCluster != 0)
	{
		m_FAT.FreeClusterChain (nFirstCluster);
	}

	pEntry->Name[0] = FAT_DIR_NAME0_FREE;

	m_Root.FreeEntry (TRUE);

	return 1;
}
