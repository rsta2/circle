//
// usbsoundcontroller.h
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
#ifndef _circle_sound_usbsoundcontroller_h
#define _circle_sound_usbsoundcontroller_h

#include <circle/sound/soundcontroller.h>
#include <circle/usb/usbaudiostreaming.h>
#include <circle/types.h>

class CUSBSoundBaseDevice;

class CUSBSoundController : public CSoundController	/// Sound controller for USB sound devices
{
public:
	CUSBSoundController (CUSBSoundBaseDevice *pSoundDevice, unsigned nDevice);
	~CUSBSoundController (void);

	boolean Probe (void) override;

	u32 GetOutputProperties (void) const override;

	boolean EnableJack (TJack Jack) override;

	boolean SetMute (TJack Jack, boolean bEnable) override;

	boolean SetVolume (TJack Jack, int ndB, TChannel Channel) override;
	const TRange GetVolumeRange (TJack Jack) const override;

private:
	// returns matching priority with jack (0..N, 0: best)
	static const unsigned NoMatch = 99;
	unsigned MatchTerminalType (u16 usTerminalType, TJack Jack);

private:
	CUSBSoundBaseDevice *m_pSoundDevice;
	unsigned m_nDevice;
	unsigned m_nInterface;

	CUSBAudioStreamingDevice *m_pStreamingDevice;
	CUSBAudioStreamingDevice::TDeviceInfo m_DeviceInfo;
};

#endif
