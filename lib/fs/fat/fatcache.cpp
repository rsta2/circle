//
// fatcache.cpp
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
#include <circle/fs/fat/fatcache.h>
#include <circle/logger.h>
#include <circle/new.h>
#include <assert.h>

#define BUFFER_MAGIC		0x4641544D
#define BUFFER_NOSECTOR		0xFFFFFFFF

#define FAULT_NO_BUFFER		0x1501
#define FAULT_READ_ERROR	0x1502
#define FAULT_WRITE_ERROR	0x1503

CFATCache::CFATCache (void)
:	m_pPartition (0)
{
	m_BufferList.pFirst = 0;
	m_BufferList.pLast = 0;
}

CFATCache::~CFATCache (void)
{
}

int CFATCache::Open (CDevice *pPartition)
{
	TFATBuffer *pBuffer;
	TFATBuffer *pPrevBuffer = 0;
	int i;
	
	assert (m_pPartition == 0);
	m_pPartition = pPartition;
	assert (m_pPartition != 0);

	for (i = 1; i <= FAT_BUFFERS; i++)
	{
		pBuffer = new (HEAP_DMA30) TFATBuffer;
		if (pBuffer == 0)
		{
			return 0;
		}

		pBuffer->nMagic    = BUFFER_MAGIC;
		pBuffer->pNext     = 0;
		pBuffer->pPrev     = pPrevBuffer;
		pBuffer->nSector   = BUFFER_NOSECTOR;
		pBuffer->nUseCount = 0;

		if (pPrevBuffer != 0)
		{
			pPrevBuffer->pNext = pBuffer;
		}

		pPrevBuffer = pBuffer;
		
		if (i == 1)
		{
			m_BufferList.pFirst = pBuffer;
		}
		else if (i == FAT_BUFFERS)
		{
			m_BufferList.pLast = pBuffer;
		}
	}

	return 1;
}

void CFATCache::Close (void)
{
	TFATBuffer *pBuffer;
	TFATBuffer *pNextBuffer;

	Flush ();

	for (pBuffer = m_BufferList.pFirst; pBuffer != 0; pBuffer = pNextBuffer)
	{
		assert (pBuffer->nMagic == BUFFER_MAGIC);

		pNextBuffer = pBuffer->pNext;

		pBuffer->nMagic = 0;

		delete [] pBuffer;
	}

	m_BufferList.pFirst = 0;
	m_BufferList.pLast = 0;
}

void CFATCache::Flush (void)
{
	TFATBuffer *pBuffer;

	m_BufferListLock.Acquire ();

	for (pBuffer = m_BufferList.pFirst; pBuffer != 0; pBuffer = pBuffer->pNext)
	{
		assert (pBuffer->nMagic == BUFFER_MAGIC);

		if (pBuffer->nSector != BUFFER_NOSECTOR)
		{
			if (pBuffer->bDirty)
			{
				m_DiskLock.Acquire ();

				m_pPartition->Seek (pBuffer->nSector * FAT_SECTOR_SIZE);
				if (m_pPartition->Write (pBuffer->Data, FAT_SECTOR_SIZE) != FAT_SECTOR_SIZE)
				{
					Fault (FAULT_WRITE_ERROR);
				}

				m_DiskLock.Release ();

				pBuffer->bDirty = 0;
			}
		}
	}

	m_BufferListLock.Release ();
}

TFATBuffer *CFATCache::GetSector (unsigned nSector, int bWriteOnly)
{
	TFATBuffer *pBuffer;

	m_BufferListLock.Acquire ();

	for (pBuffer = m_BufferList.pFirst; pBuffer != 0; pBuffer = pBuffer->pNext)
	{
		assert (pBuffer->nMagic == BUFFER_MAGIC);

		if (pBuffer->nSector == nSector)
		{
			break;
		}
	}

	if (pBuffer != 0)
	{
		MoveBufferFirst (pBuffer);

		pBuffer->nUseCount++;

		m_BufferListLock.Release ();

		return pBuffer;
	}


	for (pBuffer = m_BufferList.pLast; pBuffer != 0; pBuffer = pBuffer->pPrev)
	{
		assert (pBuffer->nMagic == BUFFER_MAGIC);

		if (pBuffer->nSector == BUFFER_NOSECTOR)
		{
			break;
		}
	}

	if (pBuffer == 0)
	{
		for (pBuffer = m_BufferList.pLast; pBuffer != 0; pBuffer = pBuffer->pPrev)
		{
			assert (pBuffer->nMagic == BUFFER_MAGIC);

			if (pBuffer->nUseCount == 0)
			{
				break;
			}
		}

		if (pBuffer == 0)
		{
			Fault (FAULT_NO_BUFFER);
			m_BufferListLock.Release ();
			return 0;
		}

		if (pBuffer->bDirty)
		{
			m_DiskLock.Acquire ();

			m_pPartition->Seek (pBuffer->nSector * FAT_SECTOR_SIZE);
			if (m_pPartition->Write (pBuffer->Data, FAT_SECTOR_SIZE) != FAT_SECTOR_SIZE)
			{
				Fault (FAULT_WRITE_ERROR);
				m_DiskLock.Release ();
				m_BufferListLock.Release ();
				return 0;
			}

			m_DiskLock.Release ();
		}

		pBuffer->nSector = BUFFER_NOSECTOR;
		assert (pBuffer->nUseCount == 0);
	}

	pBuffer->nUseCount = 1;
	assert (pBuffer->nSector == BUFFER_NOSECTOR);
	pBuffer->nSector = nSector;
	pBuffer->bDirty = 0;

	if (!bWriteOnly)
	{
		m_DiskLock.Acquire ();

		m_pPartition->Seek (nSector * FAT_SECTOR_SIZE);
		if (m_pPartition->Read (pBuffer->Data, FAT_SECTOR_SIZE) != FAT_SECTOR_SIZE)
		{
			pBuffer->nUseCount--;
			pBuffer->nSector = BUFFER_NOSECTOR;

			Fault (FAULT_READ_ERROR);
			m_DiskLock.Release ();
			m_BufferListLock.Release ();
			return 0;
		}

		m_DiskLock.Release ();
	}

	MoveBufferFirst (pBuffer);

	m_BufferListLock.Release ();

	return pBuffer;
}

void CFATCache::FreeSector (TFATBuffer *pBuffer, int bCritical)
{
	assert (pBuffer->nMagic == BUFFER_MAGIC);
	assert (pBuffer->nUseCount > 0);
	pBuffer->nUseCount--;
	if (pBuffer->nUseCount > 0)
	{
		return;
	}

	if (bCritical)
	{
#if 0
		if (pBuffer->bDirty)
		{
			m_DiskLock.Acquire ();

			m_pPartition->Seek (pBuffer->nSector * FAT_SECTOR_SIZE);
			if (m_pPartition->Write (pBuffer->Data, FAT_SECTOR_SIZE) == FAT_SECTOR_SIZE)
			{
				pBuffer->bDirty = 0;
			}

			m_DiskLock.Release ();
		}
#endif
	}
	else
	{
		m_BufferListLock.Acquire ();

		MoveBufferLast (pBuffer);

		m_BufferListLock.Release ();
	}
}

void CFATCache::MarkDirty (TFATBuffer *pBuffer)
{
	assert (pBuffer->nMagic == BUFFER_MAGIC);
	assert (pBuffer->nUseCount > 0);
	pBuffer->bDirty = 1;
}

void CFATCache::MoveBufferFirst (TFATBuffer *pBuffer)
{
	if (m_BufferList.pFirst != pBuffer)
	{
		TFATBuffer *pNext = pBuffer->pNext;
		TFATBuffer *pPrev = pBuffer->pPrev;

		pPrev->pNext = pNext;

		if (pNext)
		{
			pNext->pPrev = pPrev;
		}
		else
		{
			m_BufferList.pLast = pPrev;
		}

		m_BufferList.pFirst->pPrev = pBuffer;
		pBuffer->pNext = m_BufferList.pFirst;
		m_BufferList.pFirst = pBuffer;
		pBuffer->pPrev = 0;
	}
}

void CFATCache::MoveBufferLast (TFATBuffer *pBuffer)
{
	if (m_BufferList.pLast != pBuffer)
	{
		TFATBuffer *pNext = pBuffer->pNext;
		TFATBuffer *pPrev = pBuffer->pPrev;

		pNext->pPrev = pPrev;

		if (pPrev)
		{
			pPrev->pNext = pNext;
		}
		else
		{
			m_BufferList.pFirst = pNext;
		}

		m_BufferList.pLast->pNext = pBuffer;
		pBuffer->pPrev = m_BufferList.pLast;
		m_BufferList.pLast = pBuffer;
		pBuffer->pNext = 0;
	}
}

void CFATCache::Fault (unsigned nCode)
{
	const char *pMsg = "General error";

	switch (nCode)
	{
	case FAULT_NO_BUFFER:
		pMsg = "No buffer available";
		break;

	case FAULT_READ_ERROR:
		pMsg = "Read error";
		break;

	case FAULT_WRITE_ERROR:
		pMsg = "Write error";
		break;

	default:
		assert (0);
		break;
	}

	CLogger::Get ()->Write ("fatcache", LogPanic, pMsg);
}
