//
// usbstring.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2017  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_usbstring_h
#define _circle_usb_usbstring_h

#include <circle/usb/usb.h>
#include <circle/string.h>
#include <circle/types.h>

class CUSBDevice;

class CUSBString
{
public:
	CUSBString (CUSBDevice *pDevice);
	CUSBString (CUSBString *pParent);	// copy constructor
	~CUSBString (void);

	boolean GetFromDescriptor (u8 ucID, u16 usLanguageID);

	const char *Get (void) const;

	u16 GetLanguageID (void);

private:
	CUSBDevice *m_pDevice;

	TUSBStringDescriptor *m_pUSBString;

	CString *m_pString;
};

#endif
