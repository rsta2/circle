//
// usbfunction.h
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
#ifndef _circle_usb_usbfunction_h
#define _circle_usb_usbfunction_h

#include <circle/device.h>
#include <circle/usb/usbconfigparser.h>
#include <circle/usb/usb.h>
#include <circle/usb/usbhub.h>
#include <circle/string.h>
#include <circle/types.h>

class CUSBDevice;
class CUSBHostController;
class CUSBEndpoint;

class CUSBFunction : public CDevice
{
public:
	CUSBFunction (CUSBDevice *pDevice, CUSBConfigurationParser *pConfigParser);
	CUSBFunction (CUSBFunction *pFunction);		// copy constructor
	virtual ~CUSBFunction (void);

	virtual boolean Initialize (void);
	virtual boolean Configure (void);

	virtual boolean ReScanDevices (void);
	virtual boolean RemoveDevice (void);

	CString *GetInterfaceName (void) const;		// string deleted by caller
	u8 GetNumEndpoints (void) const;

	CUSBDevice *GetDevice (void) const;
	CUSBEndpoint *GetEndpoint0 (void) const;
	CUSBHostController *GetHost (void) const;
	
	// get next sub descriptor of ucType from interface descriptor
	const TUSBDescriptor *GetDescriptor (u8 ucType);	// returns 0 if not found
	void ConfigurationError (const char *pSource) const;

	// select a specific USB interface, called in constructor of derived class,
	// if device has been detected by vendor/product ID
	boolean SelectInterfaceByClass (u8 uchClass, u8 uchSubClass, u8 uchProtocol);

	u8 GetInterfaceNumber (void) const;
	u8 GetInterfaceClass (void) const;
	u8 GetInterfaceSubClass (void) const;
	u8 GetInterfaceProtocol (void) const;

	const TUSBInterfaceDescriptor *GetInterfaceDescriptor (void) const;

#if RASPPI >= 4
	// returns 0 if this is not a hub function
	virtual const TUSBHubInfo *GetHubInfo (void) const	{ return 0; }
#endif

private:
	CUSBDevice *m_pDevice;

	CUSBConfigurationParser *m_pConfigParser;

	TUSBInterfaceDescriptor *m_pInterfaceDesc;
};

#endif
