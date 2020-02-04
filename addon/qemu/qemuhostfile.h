//
// qemuhostfile.h
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
#ifndef _qemu_qemuhostfile_h
#define _qemu_qemuhostfile_h

#include <circle/device.h>
#include <circle/qemu.h>
#include <circle/types.h>

/// \note This class requires QEMU started with the -semihosting option to work!

class CQEMUHostFile : public CDevice	/// Accesses a file using the QEMU semihosting interface
{
public:
	/// \param pFileName File on QEMU host to be opened (default: stdout)
	/// \param bWrite    TRUE if file is written (default), FALSE if file is read
	CQEMUHostFile (const char *pFileName = SEMIHOSTING_STDIO_NAME, boolean bWrite = TRUE);

	~CQEMUHostFile (void);

	/// \return Is file open for access?
	boolean IsOpen (void) const;

	/// \param pBuffer Pointer to buffer for read data
	/// \param nCount Maximum number of bytes to be read
	/// \return Number of bytes read (0 on EOF, < 0 on error)
	int Read (void *pBuffer, size_t nCount);

	/// \param pBuffer Pointer to data to be written
	/// \param nCount Number of bytes to be written
	/// \return Number of bytes successfully written (< 0 on error)
	int Write (const void *pBuffer, size_t nCount);

private:
	TSemihostingValue m_nHandle;
};

#endif
