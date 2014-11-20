//
// smsc951x.h
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
#ifndef _smsc951x_h
#define _smsc951x_h

#include <circle/usb/netdevice.h>
#include <circle/usb/usbendpoint.h>
#include <circle/usb/usbrequest.h>
#include <circle/usb/macaddress.h>
#include <circle/types.h>

class CSMSC951xDevice : public CNetDevice
{
public:
	CSMSC951xDevice (CUSBFunction *pFunction);
	~CSMSC951xDevice (void);

	boolean Configure (void);

	const CMACAddress *GetMACAddress (void) const;

	boolean SendFrame (const void *pBuffer, unsigned nLength);
	
	// pBuffer must have size FRAME_BUFFER_SIZE
	boolean ReceiveFrame (void *pBuffer, unsigned *pResultLength);
	
private:
	boolean WriteReg (u32 nIndex, u32 nValue);
	boolean ReadReg (u32 nIndex, u32 *pValue);

#ifndef NDEBUG
	void DumpReg (const char *pName, u32 nIndex);
	void DumpRegs (void);
#endif

private:
	CUSBEndpoint *m_pEndpointBulkIn;
	CUSBEndpoint *m_pEndpointBulkOut;

	CMACAddress m_MACAddress;

	u8 *m_pTxBuffer;

	static unsigned s_nDeviceNumber;
};

#endif
