//
// usbcdcethernet.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2019  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_usbcdcethernet_h
#define _circle_usb_usbcdcethernet_h

#include <circle/netdevice.h>
#include <circle/usb/usbfunction.h>
#include <circle/usb/usbendpoint.h>
#include <circle/usb/usbrequest.h>
#include <circle/macaddress.h>
#include <circle/types.h>

class CUSBCDCEthernetDevice : public CUSBFunction, CNetDevice
{
public:
	CUSBCDCEthernetDevice (CUSBFunction *pFunction);
	~CUSBCDCEthernetDevice (void);

	boolean Configure (void);

	const CMACAddress *GetMACAddress (void) const;

	boolean SendFrame (const void *pBuffer, unsigned nLength);

	// pBuffer must have size FRAME_BUFFER_SIZE
	boolean ReceiveFrame (void *pBuffer, unsigned *pResultLength);

private:
	boolean InitMACAddress (u8 iMACAddress);

private:
	CUSBEndpoint *m_pEndpointBulkIn;
	CUSBEndpoint *m_pEndpointBulkOut;

	CMACAddress m_MACAddress;
};

#endif
