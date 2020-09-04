//
// usbprinter.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_usbprinter_h
#define _circle_usb_usbprinter_h

#include <circle/usb/usbfunction.h>
#include <circle/usb/usbendpoint.h>
#include <circle/numberpool.h>
#include <circle/types.h>

enum TUSBPrinterProtocol
{
	USBPrinterProtocolUnknown	 = 0,
	USBPrinterProtocolUnidirectional = 1,
	USBPrinterProtocolBidirectional  = 2
};

class CUSBPrinterDevice : public CUSBFunction
{
public:
	CUSBPrinterDevice (CUSBFunction *pFunction);
	~CUSBPrinterDevice (void);

	boolean Configure (void);

	int Write (const void *pBuffer, size_t nCount);

private:
	TUSBPrinterProtocol m_Protocol;

	CUSBEndpoint *m_pEndpointIn;
	CUSBEndpoint *m_pEndpointOut;

	unsigned m_nDeviceNumber;
	static CNumberPool s_DeviceNumberPool;
};

#endif
