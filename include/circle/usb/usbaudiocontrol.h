//
// usbaudiocontrol.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2022  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_usbaudiocontrol_h
#define _circle_usb_usbaudiocontrol_h

#include <circle/usb/usbfunction.h>
#include <circle/usb/usbaudiofunctopology.h>
#include <circle/numberpool.h>
#include <circle/types.h>

/// \note An instance of this driver is loaded for all USB audio streaming devices.
///	  It is not loaded for USB audio class MIDI devices.

class CUSBAudioControlDevice : public CUSBFunction	/// Driver for USB audio control devices
{
public:
	CUSBAudioControlDevice (CUSBFunction *pFunction);
	~CUSBAudioControlDevice (void);

	boolean Configure (void);

	/// \param uchInputTerminalID Terminal ID of the USB streaming Input Terminal
	/// \return Terminal type of the associated output terminal\n
	//	    (or USB_AUDIO_TERMINAL_TYPE_USB_UNDEFINED)
	u16 GetTerminalType (u8 uchInputTerminalID) const;

	/// \brief Get the clock source of an Input Terminal of the controlled device
	/// \param uchInputTerminalID Terminal ID of the USB streaming Input Terminal
	/// \return Unit ID of the associated clock source (or USB_AUDIO_UNDEFINED_UNIT_ID)
	/// \note Supported for USB Audio class version 2.00 devices only
	u8 GetClockSourceID (u8 uchInputTerminalID) const;

	/// \param uchInputTerminalID Terminal ID of the USB streaming Input Terminal
	/// \return Unit ID of the associated feature unit (or USB_AUDIO_UNDEFINED_UNIT_ID)
	u8 GetFeatureUnitID (u8 uchInputTerminalID) const;

private:
	CUSBAudioFunctionTopology m_Topology;

	unsigned m_nDeviceNumber;
	static CNumberPool s_DeviceNumberPool;
};

#endif
