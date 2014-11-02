//
// usbendpoint.h
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
#ifndef _usbendpoint_h
#define _usbendpoint_h

#include <circle/usb/usb.h>
#include <circle/usb/usbdevice.h>
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
	CUSBEndpoint (CUSBEndpoint *pEndpoint, CUSBDevice *pDevice);		// copy constructor
	~CUSBEndpoint (void);

	CUSBDevice *GetDevice (void) const;
	
	u8 GetNumber (void) const;
	TEndpointType GetType (void) const;
	boolean IsDirectionIn (void) const;

	void SetMaxPacketSize (u32 nMaxPacketSize);
	u32 GetMaxPacketSize (void) const;
	
	unsigned GetInterval (void) const;		// Milliseconds

	TUSBPID GetNextPID (boolean bStatusStage);
	void SkipPID (unsigned nPackets, boolean bStatusStage);
	void ResetPID (void);
	
private:
	CUSBDevice	*m_pDevice;
	u8		 m_ucNumber;
	TEndpointType	 m_Type;
	boolean		 m_bDirectionIn;
	u32		 m_nMaxPacketSize;
	unsigned	 m_nInterval;			// Milliseconds
	TUSBPID		 m_NextPID;
};

#endif
