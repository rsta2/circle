//
// usbmassdevice.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2022  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_usbmassdevice_h
#define _circle_usb_usbmassdevice_h

#include <circle/usb/usbfunction.h>
#include <circle/usb/usbendpoint.h>
#include <circle/fs/partitionmanager.h>
#include <circle/numberpool.h>
#include <circle/types.h>

#define UMSD_BLOCK_SIZE		512
#define UMSD_BLOCK_MASK		(UMSD_BLOCK_SIZE-1)
#define UMSD_BLOCK_SHIFT	9

#define UMSD_MAX_OFFSET		0x1FFFFFFFFFFULL		// 2TB

class CUSBBulkOnlyMassStorageDevice : public CUSBFunction
{
public:
	CUSBBulkOnlyMassStorageDevice (CUSBFunction *pFunction);
	~CUSBBulkOnlyMassStorageDevice (void);

	boolean Configure (void);

	int Read (void *pBuffer, size_t nCount);
	int Write (const void *pBuffer, size_t nCount);

	u64 Seek (u64 ullOffset);

	u64 GetSize (void) const;		// in bytes
	unsigned GetCapacity (void) const;	// in blocks

private:
	int TryRead (void *pBuffer, size_t nCount);
	int TryWrite (const void *pBuffer, size_t nCount);

	int Command (void *pCmdBlk, size_t nCmdBlkLen, void *pBuffer, size_t nBufLen, boolean bIn);

	int Reset (void);

private:
	CUSBEndpoint *m_pEndpointIn;
	CUSBEndpoint *m_pEndpointOut;

	unsigned m_nCWBTag;
	unsigned m_nBlockCount;
	u64 m_ullOffset;

	CPartitionManager *m_pPartitionManager;

	static CNumberPool s_DeviceNumberPool;
	unsigned m_nDeviceNumber;
};

#endif
