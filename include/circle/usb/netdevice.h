//
// netdevice.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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
#ifndef _netdevice_h
#define _netdevice_h

#include <circle/usb/usbfunction.h>
#include <circle/usb/macaddress.h>
#include <circle/types.h>

#define FRAME_BUFFER_SIZE	1600

class CNetDevice : public CUSBFunction
{
public:
	CNetDevice (CUSBFunction *pFunction);
	virtual ~CNetDevice (void);

	virtual boolean Configure (void) = 0;
	
	virtual const CMACAddress *GetMACAddress (void) const = 0;

	virtual boolean SendFrame (const void *pBuffer, unsigned nLength) = 0;

	// pBuffer must have size FRAME_BUFFER_SIZE
	virtual boolean ReceiveFrame (void *pBuffer, unsigned *pResultLength) = 0;
};

#endif
