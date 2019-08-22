//
// usbendpoint.h
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
#ifndef _circle_usb_usbendpoint_h
#define _circle_usb_usbendpoint_h

#include <circle/usb/usb.h>
#include <circle/usb/usbdevice.h>
#include <circle/usb/xhciendpoint.h>
#include <circle/types.h>

enum TEndpointType
{
	EndpointTypeControl,
	EndpointTypeBulk,
	EndpointTypeInterrupt,
	EndpointTypeIsochronous
};

class CUSBEndpoint
{
public:
	CUSBEndpoint (CUSBDevice *pDevice);					// control endpoint 0
	CUSBEndpoint (CUSBDevice *pDevice, const TUSBEndpointDescriptor *pDesc);
	~CUSBEndpoint (void);

	CUSBDevice *GetDevice (void) const;

	u8 GetNumber (void) const;
	TEndpointType GetType (void) const;
	boolean IsDirectionIn (void) const;

	boolean SetMaxPacketSize (u32 nMaxPacketSize);
	u32 GetMaxPacketSize (void) const;

#if RASPPI <= 3
	unsigned GetInterval (void) const;		// Milliseconds

	TUSBPID GetNextPID (boolean bStatusStage);
	void SkipPID (unsigned nPackets, boolean bStatusStage);
#endif
	void ResetPID (void);

#if RASPPI >= 4
	CXHCIEndpoint *GetXHCIEndpoint (void);
#endif

private:
	CUSBDevice	*m_pDevice;
	u8		 m_ucNumber;
	TEndpointType	 m_Type;
	boolean		 m_bDirectionIn;
	u32		 m_nMaxPacketSize;
#if RASPPI <= 3
	unsigned	 m_nInterval;			// Milliseconds
	TUSBPID		 m_NextPID;
#endif

#if RASPPI >= 4
	CXHCIEndpoint	*m_pXHCIEndpoint;
#endif
};

#endif
