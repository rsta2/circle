//
// memdevice.h
// A CDevice backed by RAM, emulating a FAT partition
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2019  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_memdevice_h
#define _circle_memdevice_h

#include <circle/alloc.h>
#include <circle/util.h>
#include <circle/device.h>
#include <circle/types.h>

class CMemDevice: public CDevice
{
public:
	CMemDevice (size_t nsize);
	~CMemDevice (void);

	/// \param pBuffer Buffer, where read data will be placed
	/// \param nCount Maximum number of bytes to be read
	/// \return Number of read bytes or < 0 on failure
	int Read (void *pBuffer, size_t nCount);

	/// \param pBuffer Buffer, from which data will be fetched for write
	/// \param nCount Number of bytes to be written
	/// \return Number of written bytes or < 0 on failure
	int Write (const void *pBuffer, size_t nCount);

	/// \param ullOffset Byte offset from start
	/// \return The resulting offset, (u64) -1 on error
	/// \note Supported by block devices only
	u64 Seek (u64 ullOffset);

	/// \return Total byte size of a block device, (u64) -1 on error
	/// \note Supported by block devices only
	u64 GetSize (void) const;

	/// \param ulCmd The IOCtl command to invoke
	/// \param pData Depends on command, used to return command specific data
	/// \return Zero on success, or error code on failure
	int IOCtl (unsigned long ulCmd, void *pData);

	/// \return TRUE on successful device removal
	boolean RemoveDevice (void);

private:
    char *m_mem;
    u64 m_ulloffset;
    u64 m_nsize;
	static u8 m_Sector0[512],m_Sector2[512],m_Sector10[9],m_Sector34[349],m_Sector66[253],m_Sector70[12],m_Sector74[75],
	          m_Sector78[132];
};
#endif
