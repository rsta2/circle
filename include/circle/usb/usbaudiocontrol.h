//
// usbaudiocontrol.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2022-2023  R. Stange <rsta2@o2online.de>
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

	/// \return Device number of this USB audio device
	unsigned GetDeviceNumber (void) const;
	/// \return Sub-device number for next interface of this USB audio streaming device
	unsigned GetNextStreamingSubDeviceNumber (void);

	/// \param uchEntityID ID of an audio Entity of the controlled device
	/// \param bUpstream Search upstream (or downstream)
	/// \return Terminal type of the associated terminating terminal in search order\n
	//	    (or USB_AUDIO_TERMINAL_TYPE_USB_UNDEFINED)
	u16 GetTerminalType (u8 uchEntityID, boolean bUpstream) const;

	/// \brief Get the clock source of a Terminal of the controlled device
	/// \param uchTerminalID Terminal ID of the USB streaming Terminal
	/// \return Unit ID of the associated clock source (or USB_AUDIO_UNDEFINED_UNIT_ID)
	/// \note Supported for USB Audio class version 2.00 devices only
	u8 GetClockSourceID (u8 uchTerminalID) const;

	/// \param uchOutputTerminalID Terminal ID of the USB streaming Output Terminal
	/// \return Unit ID of the associated selector unit (or USB_AUDIO_UNDEFINED_UNIT_ID)
	u8 GetSelectorUnitID (u8 uchOutputTerminalID) const;

	/// \param uchEntityID ID of an audio Entity of the controlled device
	/// \return Number of sources of the Entity (or 0 on error)
	unsigned GetNumSources (u8 uchEntityID) const;

	/// \param uchEntityID ID of an audio Entity of the controlled device
	/// \param nIndex Index of the source pin (0 .. NumSources-1)
	/// \return Entity ID of a source of the given Entity (or USB_AUDIO_UNDEFINED_UNIT_ID)
	u8 GetSourceID (u8 uchEntityID, unsigned nIndex) const;

	/// \param uchEntityID ID of an audio Entity of the controlled device
	/// \param bUpstream Search upstream (or downstream)
	/// \return Unit ID of the associated Feature Unit in search order\n
	///	    (or USB_AUDIO_UNDEFINED_UNIT_ID)
	/// \note If uchEntityID references a Feature Unit itself, it is returned.
	u8 GetFeatureUnitID (u8 uchEntityID, boolean bUpstream) const;

	/// \brief Get the clock selector with index nIndex
	/// \param nIndex 0-based index of the clock selector
	/// \return Unit ID of the clock selector (or USB_AUDIO_UNDEFINED_UNIT_ID)
	/// \note Supported for USB Audio class version 2.00 devices only
	u8 GetClockSelectorID (unsigned nIndex = 0);

	/// \param uchFeatureUnitID Unit ID of a Feature Unit of the controlled device
	/// \param nChannel Audio channel (0: master, 1: normally front left, 2: right)
	/// \param Control Control selector (see: <circle/usb/usbaudiofunctopology.h>)
	/// \return Selected control is present and read/writable
	boolean IsControlSupported (u8 uchFeatureUnitID, unsigned nChannel,
				    CUSBAudioFeatureUnit::TControl Control) const;

private:
	CUSBAudioFunctionTopology m_Topology;

	unsigned m_nDeviceNumber;
	static CNumberPool s_DeviceNumberPool;

	unsigned m_nNextStreamingSubDeviceNumber;
};

#endif
