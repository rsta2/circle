//
// writebuffer.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2020  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_writebuffer_h
#define _circle_writebuffer_h

#include <circle/device.h>
#include <circle/spinlock.h>
#include <circle/types.h>

class CWriteBufferDevice : public CDevice	/// Filter for buffered write to (e.g. screen) device
{
public:
	/// \param pDevice     The device, the output is sent to (e.g. screen)
	/// \param nBufferSize Size of the internal ring buffer (must be a power of 2)
	CWriteBufferDevice (CDevice *pDevice, size_t nBufferSize = 4096);

	~CWriteBufferDevice (void);

	/// \brief Write data into the internal ring buffer.
	/// \param pBuffer Pointer to the data to be written
	/// \param nCount  Number of bytes to be written
	/// \return Number of written bytes or < 0 on failure
	/// \note This method is callable from any core and at IRQ_LEVEL too.
	int Write (const void *pBuffer, size_t nCount);

	/// \brief Write contents from ring buffer to the output device.
	/// \param nMaxBytes Maximum number of bytes written to the device at once
	/// \note This method must be called at TASK_LEVEL or from a secondary core (1-3).
	void Update (size_t nMaxBytes = 100);

private:
	CDevice *m_pDevice;
	size_t m_nBufferSize;

	u8 *m_pBuffer;
	volatile unsigned m_nInPtr;
	volatile unsigned m_nOutPtr;

	CSpinLock m_SpinLock;
};

#endif
