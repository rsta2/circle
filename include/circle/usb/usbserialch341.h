//
// usbserialch341.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2020  H. Kocevar <hinxx@protonmail.com>
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
#ifndef _circle_usb_usbserialch341_h
#define _circle_usb_usbserialch341_h

#include <circle/usb/usbserial.h>
#include <circle/usb/usbdevicefactory.h>
#include <circle/types.h>

class CUSBSerialCH341Device : public CUSBSerialDevice
{
public:
	CUSBSerialCH341Device (CUSBFunction *pFunction);
	~CUSBSerialCH341Device (void);

	boolean Configure (void);
	boolean SetBaudRate (unsigned nBaudRate);
	boolean SetLineProperties (TUSBSerialDataBits nDataBits,
				   TUSBSerialParity nParity, TUSBSerialStopBits nStopBits);

	static const TUSBDeviceID *GetDeviceIDTable (void);
};

#endif
