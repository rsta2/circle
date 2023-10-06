//
// usbhostcontroller.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2023  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_usbhostcontroller_h
#define _circle_usb_usbhostcontroller_h

#include <circle/usb/usbcontroller.h>
#include <circle/usb/usb.h>
#include <circle/usb/usbendpoint.h>
#include <circle/usb/usbrequest.h>
#include <circle/ptrlist.h>
#include <circle/spinlock.h>
#include <circle/types.h>

// Timeouts are supported on interrupt endpoints only!

#define USB_TIMEOUT_NONE	0	// Wait forever

class CUSBHCIRootPort;
class CUSBStandardHub;
class CUSBDevice;

class CUSBHostController : public CUSBController
{
public:
	CUSBHostController (boolean bPlugAndPlay);
	virtual ~CUSBHostController (void);
	
	// returns resulting length or < 0 on failure
	int GetDescriptor (CUSBEndpoint *pEndpoint,
			   unsigned char ucType, unsigned char ucIndex,
			   void *pBuffer, unsigned nBufSize,
			   unsigned char ucRequestType = REQUEST_IN,
			   unsigned short wIndex = 0);		// endpoint, interface or language ID
	
	boolean SetAddress (CUSBEndpoint *pEndpoint, u8 ucDeviceAddress);
	
	boolean SetConfiguration (CUSBEndpoint *pEndpoint, u8 ucConfigurationValue);
	
	// returns resulting length or < 0 on failure
	int ControlMessage (CUSBEndpoint *pEndpoint,
			    u8 ucRequestType, u8 ucRequest, u16 usValue, u16 usIndex,
			    void *pData, u16 usDataSize);

	// returns resulting length or < 0 on failure
	int Transfer (CUSBEndpoint *pEndpoint, void *pBuffer, unsigned nBufSize,
		      unsigned nTimeoutMs = USB_TIMEOUT_NONE);

public:
	virtual boolean SubmitBlockingRequest (CUSBRequest *pURB,
					       unsigned nTimeoutMs = USB_TIMEOUT_NONE) = 0;

	virtual boolean SubmitAsyncRequest (CUSBRequest *pURB,
					    unsigned nTimeoutMs = USB_TIMEOUT_NONE) = 0;

	virtual void CancelDeviceTransactions (CUSBDevice *pUSBDevice) {}

public:
	static boolean IsPlugAndPlay (void);

	// must be called from TASK_LEVEL, if Plug-and-Play is enabled
	// returns TRUE if device tree might have been updated (always TRUE on first call)
	boolean UpdatePlugAndPlay (void) override;

	static boolean IsActive (void)
	{
		return s_pThis != 0 ? TRUE : FALSE;
	}

	static CUSBHostController *Get (void);

protected:
	void PortStatusChanged (CUSBHCIRootPort *pRootPort);
	friend class CXHCIRootPort;

private:
	void PortStatusChanged (CUSBStandardHub *pHub);
	friend class CUSBStandardHub;

private:
	static boolean s_bPlugAndPlay;
	boolean m_bFirstUpdateCall;

	CPtrList  m_HubList;
	CSpinLock m_SpinLock;

	static CUSBHostController *s_pThis;
};

#endif
