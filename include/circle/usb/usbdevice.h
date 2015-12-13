//
// usbdevice.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2015  R. Stange <rsta2@o2online.de>
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
#ifndef _usbdevice_h
#define _usbdevice_h

#include <circle/usb/usb.h>
#include <circle/usb/usbconfigparser.h>
#include <circle/usb/usbfunction.h>
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
class CUSBEndpoint;

class CUSBDevice
{
public:
	CUSBDevice (CUSBHostController *pHost, TUSBSpeed Speed,
		    boolean bSplitTransfer, u8 ucHubAddress, u8 ucHubPortNumber);
	virtual ~CUSBDevice (void);
	
	virtual boolean Initialize (void);			// onto address state (phase 1)
	virtual boolean Configure (void);			// onto configured state (phase 2)

	CString *GetName (TDeviceNameSelector Selector) const;	// string deleted by caller
	CString *GetNames (void) const;				// string deleted by caller
	
	u8 GetAddress (void) const;
	TUSBSpeed GetSpeed (void) const;

	boolean IsSplit (void) const;
	u8 GetHubAddress (void) const;
	u8 GetHubPortNumber (void) const;

	CUSBEndpoint *GetEndpoint0 (void) const;
	CUSBHostController *GetHost (void) const;
	
	const TUSBDeviceDescriptor *GetDeviceDescriptor (void) const;
	const TUSBConfigurationDescriptor *GetConfigurationDescriptor (void) const; // default config

	// get next sub descriptor of ucType from configuration descriptor
	const TUSBDescriptor *GetDescriptor (u8 ucType);	// returns 0 if not found
	void ConfigurationError (const char *pSource) const;

	void LogWrite (TLogSeverity Severity, const char *pMessage, ...);

private:
	void SetAddress (u8 ucAddress);

private:
	CUSBHostController *m_pHost;
	u8		    m_ucAddress;
	TUSBSpeed	    m_Speed;
	CUSBEndpoint	   *m_pEndpoint0;

	boolean		    m_bSplitTransfer;
	u8		    m_ucHubAddress;
	u8		    m_ucHubPortNumber;
	
	TUSBDeviceDescriptor	    *m_pDeviceDesc;
	TUSBConfigurationDescriptor *m_pConfigDesc;

	CUSBConfigurationParser *m_pConfigParser;

	CUSBFunction *m_pFunction[USBDEV_MAX_FUNCTIONS];

	static u8 s_ucNextAddress;
};

#endif
