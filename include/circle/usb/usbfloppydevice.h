//
// usbfloppydevice.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2024  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_usbfloppydevice_h
#define _circle_usb_usbfloppydevice_h

#include <circle/usb/usbfunction.h>
#include <circle/usb/usbendpoint.h>
#include <circle/numberpool.h>
#include <circle/types.h>

class CUSBFloppyDiskDevice : public CUSBFunction /// Driver for USB floppy disk devices (CBI)
{
public:
	CUSBFloppyDiskDevice (CUSBFunction *pFunction);
	~CUSBFloppyDiskDevice (void);

	boolean Configure (void);

	int Read (void *pBuffer, size_t nCount);
	int Write (const void *pBuffer, size_t nCount);

	u64 Seek (u64 ullOffset);

	u64 GetSize (void) const;

private:
	int WaitUnitReady (void);

	int TryRead (void *pBuffer, size_t nCount);
	int TryWrite (const void *pBuffer, size_t nCount);

	int Command (void *pCmdBlk, size_t nCmdBlkLen, void *pBuffer, size_t nBufLen, boolean bIn);

	int Reset (void);

private:
	CUSBEndpoint *m_pEndpointIn;
	CUSBEndpoint *m_pEndpointOut;
	CUSBEndpoint *m_pEndpointInterrupt;

	unsigned m_nBlockCount;
	u64 m_ullOffset;

	static CNumberPool s_DeviceNumberPool;
	unsigned m_nDeviceNumber;
};

#endif
