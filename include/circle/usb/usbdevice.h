//
// usbdevice.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2022  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_usbdevice_h
#define _circle_usb_usbdevice_h

#include <circle/usb/usb.h>
#include <circle/usb/usbconfigparser.h>
#include <circle/usb/usbfunction.h>
#include <circle/numberpool.h>
#include <circle/logger.h>
#include <circle/string.h>
#include <circle/types.h>

#define USBDEV_MAX_FUNCTIONS	10

enum TDeviceNameSelector		// do not change this order
{
	DeviceNameVendor,
	DeviceNameDevice,
	DeviceNameUnknown
};

class CUSBHostController;
class CUSBHCIRootPort;
class CUSBStandardHub;
class CUSBEndpoint;

class CUSBDevice
{
public:
	CUSBDevice (CUSBHostController *pHost, TUSBSpeed Speed, CUSBHCIRootPort *pRootPort);
	CUSBDevice (CUSBHostController *pHost, TUSBSpeed Speed,
		    CUSBStandardHub *pHub, unsigned nHubPortIndex);
	virtual ~CUSBDevice (void);
	
	virtual boolean Initialize (void);	// onto address state (phase 1)
	virtual boolean Configure (void);	// onto configured state (phase 2)

	boolean ReScanDevices (void);
	boolean RemoveDevice (void);

	CString *GetName (TDeviceNameSelector Selector) const;	// string deleted by caller
	CString *GetNames (void) const;				// string deleted by caller

	u8 GetAddress (void) const;		// xHCI: slot ID
	TUSBSpeed GetSpeed (void) const;

	boolean IsSplit (void) const;
	u8 GetHubAddress (void) const;		// xHCI: slot ID
	u8 GetHubPortNumber (void) const;
	CUSBDevice *GetTTHubDevice (void) const;

	CUSBEndpoint *GetEndpoint0 (void) const;
	CUSBHostController *GetHost (void) const;
	
	const TUSBDeviceDescriptor *GetDeviceDescriptor (void) const;
	const TUSBConfigurationDescriptor *GetConfigurationDescriptor (void) const; // default config

	// get next sub descriptor of ucType from configuration descriptor
	const TUSBDescriptor *GetDescriptor (u8 ucType);	// returns 0 if not found
	void ConfigurationError (const char *pSource) const;

	// nIndex is 0..USBDEV_MAX_FUNCTIONS-1, returns 0 for an empty slot
	CUSBFunction *GetFunction (unsigned nIndex);

	void LogWrite (TLogSeverity Severity, const char *pMessage, ...);

#if RASPPI >= 4
	virtual boolean EnableHubFunction (void) = 0;

	unsigned GetRootHubPortID (void) const		{ return m_nRootHubPortID; }
	u32 GetRouteString (void) const			{ return m_nRouteString; }

	// returns 0 if this is not a hub device
	const TUSBHubInfo *GetHubInfo (void) const
	{
		return m_pFunction[0] != 0 ? m_pFunction[0]->GetHubInfo () : 0;
	}
#endif

protected:
	void SetAddress (u8 ucAddress);		// xHCI: set slot ID

private:
#if RASPPI >= 4
	static u32 AppendPortToRouteString (u32 nRouteString, unsigned nPort);
#endif

private:
	CUSBHostController *m_pHost;
	CUSBHCIRootPort	   *m_pRootPort;	// the root port, this device is connected to
	CUSBStandardHub	   *m_pHub;		// alternatively the hub, this device is connected to
	unsigned	    m_nHubPortIndex;	//	the 0-based index at this hub

	u8		    m_ucAddress;
	TUSBSpeed	    m_Speed;
	CUSBEndpoint	   *m_pEndpoint0;

	boolean		    m_bSplitTransfer;
	u8		    m_ucHubAddress;
	u8		    m_ucHubPortNumber;
	CUSBDevice	   *m_pTTHubDevice;

	TUSBDeviceDescriptor	    *m_pDeviceDesc;
	TUSBConfigurationDescriptor *m_pConfigDesc;

	CUSBConfigurationParser *m_pConfigParser;

	CUSBFunction *m_pFunction[USBDEV_MAX_FUNCTIONS];

#if RASPPI >= 4
	unsigned m_nRootHubPortID;
	u32	 m_nRouteString;
#endif

#if RASPPI <= 3
	static CNumberPool s_DeviceAddressPool;
#endif
};

#endif
