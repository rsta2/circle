//
// usbserialcdc.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2020-2021  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_usbserialcdc_h
#define _circle_usb_usbserialcdc_h

#include <circle/usb/usbserial.h>
#include <circle/types.h>

class CUSBSerialCDCDevice : public CUSBSerialDevice
{
public:
	CUSBSerialCDCDevice (CUSBFunction *pFunction);
	~CUSBSerialCDCDevice (void);

	boolean Configure (void);

	boolean SetBaudRate (unsigned nBaudRate);
	boolean SetLineProperties (TUSBSerialDataBits DataBits, TUSBSerialParity Parity,
				   TUSBSerialStopBits StopBits);

private:
	boolean SetLineCoding (void);

private:
	u8 m_ucCommunicationsInterfaceNumber;
	boolean m_bInterfaceOK;
};

#endif
