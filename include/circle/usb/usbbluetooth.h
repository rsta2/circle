//
// usbbluetooth.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2020  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_usbbluetooth_h
#define _circle_usb_usbbluetooth_h

#include <circle/usb/usbfunction.h>
#include <circle/usb/usbendpoint.h>
#include <circle/usb/usbrequest.h>
#include <circle/numberpool.h>
#include <circle/types.h>

typedef void TBTHCIEventHandler (const void *pBuffer, unsigned nLength);

class CUSBBluetoothDevice : public CUSBFunction
{
public:
	CUSBBluetoothDevice (CUSBFunction *pFunction);
	~CUSBBluetoothDevice (void);

	boolean Configure (void);

	boolean SendHCICommand (const void *pBuffer, unsigned nLength);

	void RegisterHCIEventHandler (TBTHCIEventHandler *pHandler);

private:
	boolean StartRequest (void);

	void CompletionRoutine (CUSBRequest *pURB);
	static void CompletionStub (CUSBRequest *pURB, void *pParam, void *pContext);

private:
	CUSBEndpoint *m_pEndpointInterrupt;
	CUSBEndpoint *m_pEndpointBulkIn;
	CUSBEndpoint *m_pEndpointBulkOut;

	u8 *m_pEventBuffer;

	TBTHCIEventHandler *m_pEventHandler;

	unsigned m_nDeviceNumber;
	static CNumberPool s_DeviceNumberPool;
};

#endif
