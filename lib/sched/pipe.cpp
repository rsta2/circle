//
// pipe.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2026  R. Stange <rsta2@gmx.net>
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
#include <circle/sched/pipe.h>
#include <assert.h>

//// CPipeFile ////////////////////////////////////////////////////////////////

CPipeFile::CPipeFile (CPipe *pPipe, TDirection Direction)
:	m_pPipe (pPipe),
	m_Direction (Direction),
	m_nOpenCount (0)
{
	assert (m_pPipe);
}

CPipeFile::~CPipeFile (void)
{
	assert (!m_nOpenCount);

	assert (m_pPipe);
	m_pPipe->Close (this);
	m_pPipe = nullptr;
}

void CPipeFile::Open (void)
{
	m_nOpenCount++;
}

void CPipeFile::Close (void)
{
	assert (m_nOpenCount);
	if (!--m_nOpenCount)
	{
		delete this;
	}
}

int CPipeFile::Read (void *pBuffer, size_t nCount)
{
	assert (m_nOpenCount);

	if (m_Direction != Reader)
	{
		return 0;
	}

	assert (m_pPipe);
	return m_pPipe->Read (pBuffer, nCount);
}

int CPipeFile::Write (const void *pBuffer, size_t nCount)
{
	assert (m_nOpenCount);

	if (m_Direction != Writer)
	{
		return 0;
	}

	assert (m_pPipe);
	return m_pPipe->Write (pBuffer, nCount);
}

CPipeFile::TStatus CPipeFile::GetStatus (void) const
{
	assert (m_nOpenCount);

	assert (m_pPipe);
	TStatus Result = m_pPipe->GetStatus ();

	// Set the "opposite direction" status to "not ready"
	if (m_Direction == Reader)
	{
		Result.bWriteReady = FALSE;
	}
	else
	{
		assert (m_Direction == Writer);
		Result.bReadReady = FALSE;
	}

	return Result;
}

void CPipeFile::SetBlocking (boolean bOn)
{
	assert (m_pPipe);
	m_pPipe->SetBlocking (m_Direction, bOn);
}

//// CPipe ////////////////////////////////////////////////////////////////////

CPipe::CPipe (boolean bAutoOpen, unsigned nFIFOSize, unsigned nAtomicWrite)
:	m_pReader (new CPipeFile (this, CPipeFile::Reader)),
	m_pWriter (new CPipeFile (this, CPipeFile::Writer)),
	m_nFIFOSize (nFIFOSize),
	m_nAtomicWrite (nAtomicWrite),
	m_pBuffer (nullptr),
	m_nInPtr (0),
	m_nOutPtr (0),
	m_bReadBlocking (TRUE),
	m_bWriteBlocking (TRUE),
	m_ReadEvent (TRUE),
	m_WriteEvent (TRUE)
{
	assert (m_pReader);
	assert (m_pWriter);

	assert (m_nFIFOSize > 1);
	assert (m_nFIFOSize > m_nAtomicWrite);
	assert (m_nFIFOSize <= 0x80000);		// 512K should be enough
	m_pBuffer = new u8[m_nFIFOSize];
	assert (m_pBuffer);

	if (bAutoOpen)
	{
		m_pReader->Open ();
		m_pWriter->Open ();
	}
}


CPipe::~CPipe (void)
{
	delete [] m_pBuffer;
	m_pBuffer = nullptr;
	m_nFIFOSize = 0;
}

void CPipe::Close (CPipeFile *pPipeFile)
{
	if (pPipeFile == m_pReader)
	{
		// Kick eventually waiting writers to return -NoReader
		if (m_pWriter)
		{
			m_WriteEvent.Set ();
		}

		m_pReader = nullptr;
	}
	else
	{
		assert (pPipeFile == m_pWriter);

		// Kick eventually waiting readers to return 0 (EOF)
		if (m_pReader)
		{
			m_ReadEvent.Set ();
		}

		m_pWriter = nullptr;
	}

	if (   !m_pReader
	    && !m_pWriter)
	{
		delete this;
	}
}

int CPipe::Read (void *pBuffer, size_t nCount)
{
	assert (pBuffer);
	assert (nCount);

	int nResult = 0;

	while (!nResult)
	{
		unsigned nBytes = GetBytesAvailable ();
		if (!nBytes)				// if FIFO is empty
		{
			if (!m_pWriter)			// and no writer any more
			{
				return 0;		// return EOF
			}

			if (!m_bReadBlocking)
			{
				return -CPipeFile::WouldBlock;
			}

			m_ReadEvent.Clear ();
			m_ReadEvent.Wait ();
		}

		nBytes = GetBytesAvailable ();
		if (nBytes)
		{
			if (nBytes < nCount)
			{
				nCount = nBytes;
			}

			FIFORead (pBuffer, nCount);

			nResult = nCount;

			m_WriteEvent.Set ();		// Kick for space waiting writers
		}
	}

	return nResult;
}

int CPipe::Write (const void *pBuffer, size_t nCount)
{
	assert (pBuffer);
	u8 *p = static_cast<u8 *> (const_cast<void *> (pBuffer));

	int nResult = 0;

	if (!m_pReader)
	{
		return -CPipeFile::NoReader;
	}

	// Messages with a length of not larger than m_nAtomicWrite
	// are sent atomically.
	boolean bAtomic = nCount <= m_nAtomicWrite;

	while (nCount)
	{
		unsigned nFree = GetFreeSpace ();
		if (   (!bAtomic && !nFree)		// not atomically requires one byte free
		    || nFree < nCount)			// nCount free otherwise
		{
			if (!m_bWriteBlocking)
			{
				return -CPipeFile::WouldBlock;
			}

			m_WriteEvent.Clear ();
			m_WriteEvent.Wait ();
		}

		nFree = GetFreeSpace ();
		if (!nFree)				// Kicked without space -> reader has closed
		{
			assert (!m_pReader);
			return -CPipeFile::NoReader;
		}

		if (bAtomic && nFree < nCount)		// Still not enough automic space?
		{
			continue;			// Continue to wait
		}

		if (nFree > nCount)
		{
			nFree = nCount;
		}

		FIFOWrite (p, nFree);

		p += nFree;
		nCount -= nFree;
		nResult += nFree;

		m_ReadEvent.Set ();			// Kick for data waiting readers
	}

	return nResult;
}

CPipeFile::TStatus CPipe::GetStatus (void) const
{
	CPipeFile::TStatus Result {FALSE, FALSE, FALSE};

	Result.bReadReady = !m_pWriter || GetBytesAvailable () > 0;
	Result.bWriteReady = !m_pReader || GetFreeSpace () > 0;

	return Result;
}

void CPipe::SetBlocking (CPipeFile::TDirection Direction, boolean bOn)
{
	if (Direction == CPipeFile::Reader)
	{
		m_bReadBlocking = bOn;
	}
	else
	{
		m_bWriteBlocking = bOn;
	}
}

unsigned CPipe::GetFreeSpace (void) const
{
	assert (m_nFIFOSize > 1);
	assert (m_nInPtr < m_nFIFOSize);
	assert (m_nOutPtr < m_nFIFOSize);

	if (m_nOutPtr <= m_nInPtr)
	{
		return m_nFIFOSize+m_nOutPtr-m_nInPtr-1;
	}

	return m_nOutPtr-m_nInPtr-1;
}

void CPipe::FIFOWrite (const void *pBuffer, unsigned nLength)
{
	assert (nLength > 0);
	assert (GetFreeSpace () >= nLength);

	unsigned char *p = (unsigned char *) pBuffer;
	assert (p != 0);
	assert (m_pBuffer != 0);

	while (nLength--)
	{
		m_pBuffer[m_nInPtr++] = *p++;
		m_nInPtr %= m_nFIFOSize;
	}
}

unsigned CPipe::GetBytesAvailable (void) const
{
	assert (m_nFIFOSize > 1);
	assert (m_nInPtr < m_nFIFOSize);
	assert (m_nOutPtr < m_nFIFOSize);

	if (m_nInPtr < m_nOutPtr)
	{
		return m_nFIFOSize+m_nInPtr-m_nOutPtr;
	}

	return m_nInPtr-m_nOutPtr;
}

void CPipe::FIFORead (void *pBuffer, unsigned nLength)
{
	assert (nLength > 0);
	assert (GetBytesAvailable () >= nLength);

	unsigned char *p = (unsigned char *) pBuffer;
	assert (p != 0);
	assert (m_pBuffer != 0);

	while (nLength--)
	{
		*p++ = m_pBuffer[m_nOutPtr++];
		m_nOutPtr %= m_nFIFOSize;
	}
}
