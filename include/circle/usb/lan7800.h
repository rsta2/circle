//
// lan7800.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2018-2019  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_lan7800_h
#define _circle_usb_lan7800_h

#include <circle/netdevice.h>
#include <circle/usb/usbfunction.h>
#include <circle/usb/usbendpoint.h>
#include <circle/usb/usbrequest.h>
#include <circle/macaddress.h>
#include <circle/timer.h>
#include <circle/types.h>

class CLAN7800Device : public CUSBFunction, CNetDevice
{
public:
	CLAN7800Device (CUSBFunction *pFunction);
	~CLAN7800Device (void);

	boolean Configure (void);

	const CMACAddress *GetMACAddress (void) const;

	boolean SendFrame (const void *pBuffer, unsigned nLength);
	
	// pBuffer must have size FRAME_BUFFER_SIZE
	boolean ReceiveFrame (void *pBuffer, unsigned *pResultLength);

	// returns TRUE if PHY link is up
	boolean IsLinkUp (void);
	
	TNetDeviceSpeed GetLinkSpeed (void);

private:
	boolean InitMACAddress (void);
	boolean InitPHY (void);

	boolean PHYWrite (u8 uchIndex, u16 usValue);
	boolean PHYRead (u8 uchIndex, u16 *pValue);

	// wait until register 'nIndex' has value 'nCompare' with mask 'nMask' applied,
	// check the register each 'nDelayMicros' microseconds, timeout after 'nTimeoutHZ' ticks
	boolean WaitReg (u32 nIndex, u32 nMask, u32 nCompare = 0,
			 unsigned nDelayMicros = 1000, unsigned nTimeoutHZ = HZ);

	boolean ReadWriteReg (u32 nIndex, u32 nOrMask, u32 nAndMask = ~0U);
	boolean WriteReg (u32 nIndex, u32 nValue);
	boolean ReadReg (u32 nIndex, u32 *pValue);

private:
	CUSBEndpoint *m_pEndpointBulkIn;
	CUSBEndpoint *m_pEndpointBulkOut;

	CMACAddress m_MACAddress;
};

#endif
