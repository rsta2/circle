//
// usbcdcethernet.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2025  R. Stange <rsta2@gmx.net>
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
#include <circle/synchronize.h>
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

	boolean SetMulticastFilter (const u8 Groups[][MAC_ADDRESS_SIZE]);

private:
	static void CompletionRoutine (CUSBRequest *pURB, void *pParam, void *pContext);

	u8 GetMACAddressStringIndex (void);	// returns 0 on error

	boolean InitMACAddress (u8 iMACAddress);

private:
	u8 m_uchControlInterface;
	u8 m_iMACAddress;
	boolean m_bInterfaceOK;

	CUSBEndpoint *m_pEndpointBulkIn;
	CUSBEndpoint *m_pEndpointBulkOut;

	CMACAddress m_MACAddress;

	CUSBRequest *volatile m_pURB;
	DMA_BUFFER (u8, m_RxBuffer, FRAME_BUFFER_SIZE);
	volatile unsigned m_nRxLength;
};

#endif
