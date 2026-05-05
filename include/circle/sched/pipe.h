//
// pipe.h
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
#ifndef _circle_sched_pipe_h
#define _circle_sched_pipe_h

#include <circle/sched/synchronizationevent.h>
#include <circle/types.h>

class CPipe;

/// \note Do not directly instantiate this class, use CPipe instead!

class CPipeFile		/// Read or Write endpoint of a CPipe channel
{
public:
	enum TResult	///< Returned as a negative value
	{
		WouldBlock = 1,	///< In non-blocking mode, when not ready
		NoReader	///< Write() returns this, when all readers have been closed
	};

public:
	/// \brief To be called, before the pipe file is reused,\n
	///	   or initially if bAutoOpen was not set.
	void Open (void);

	/// \brief To be called, when the pipe file is not used any more.
	void Close (void);

	/// \param pBuffer Buffer, where read data will be placed
	/// \param nCount Maximum number of bytes to be read
	/// \return Number of read bytes or -WouldBlock when FIFO is empty and non-blocking is off
	int Read (void *pBuffer, size_t nCount);

	/// \param pBuffer Buffer, from which data will be fetched for write
	/// \param nCount Number of bytes to be written
	/// \return Number of written bytes or -WouldBlock when FIFO is full and non-blocking off,\n
	///	    -NoReader when all readers have been closed
	int Write (const void *pBuffer, size_t nCount);

	struct TStatus
	{
		boolean bReadReady;	///< Ready to read without blocking
		boolean bWriteReady;	///< Ready to write without blocking
		boolean bException;	///< Exception arrived (always FALSE)
	};

	/// \return Pipe status
	TStatus GetStatus (void) const;

	/// \param bOn Set to FALSE to immediately return from Read() and Write(),\n
	///	       when pipe is not ready (default TRUE)
	void SetBlocking (boolean bOn);

private:
	enum TDirection
	{
		Reader,
		Writer
	};

	CPipeFile (CPipe *pPipe, TDirection Direction);
	~CPipeFile (void);
	void PeerClosed (void);
	friend class CPipe;

private:
	CPipe *m_pPipe;
	TDirection m_Direction;
	unsigned m_nOpenCount;
};

class CPipe	/// Unidirectional interprocess communication channel using a FIFO
{
public:
	/// \param bAutoOpen Set to FALSE to not automatically open reader and writer one time
	/// \param nFIFOSize Size of the internal FIFO (default 16K)
	/// \param nAtomicWrite Number of bytes to be send atomically (PIPE_BUF)
	/// \note Create this with the new operator, don't delete it!
	CPipe (boolean bAutoOpen = TRUE, unsigned nFIFOSize = 0x4000, unsigned nAtomicWrite = 512);

	/// \return Pointer to the reader object
	CPipeFile *GetReader (void) const		{ return m_pReader; }
	/// \return Pointer to the writer object
	CPipeFile *GetWriter (void) const		{ return m_pWriter; }

private:
	~CPipe (void);
	void Close (CPipeFile *pPipeFile);
	int Read (void *pBuffer, size_t nCount);
	int Write (const void *pBuffer, size_t nCount);
	CPipeFile::TStatus GetStatus (void) const;
	void SetBlocking (CPipeFile::TDirection Direction, boolean bOn);
	friend class CPipeFile;

	// FIFO handling
	unsigned GetFreeSpace (void) const;
	void FIFOWrite (const void *pBuffer, unsigned nLength);
	unsigned GetBytesAvailable (void) const;
	void FIFORead (void *pBuffer, unsigned nLength);

private:
	CPipeFile *m_pReader;
	CPipeFile *m_pWriter;

	unsigned m_nFIFOSize;
	unsigned m_nAtomicWrite;
	u8 *m_pBuffer;
	unsigned m_nInPtr;
	unsigned m_nOutPtr;

	boolean m_bReadBlocking;
	boolean m_bWriteBlocking;
	CSynchronizationEvent m_ReadEvent;
	CSynchronizationEvent m_WriteEvent;
};

#endif
