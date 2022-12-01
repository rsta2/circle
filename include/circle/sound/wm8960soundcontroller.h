//
// wm8960soundcontroller.h
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
#ifndef _circle_sound_wm8960soundcontroller_h
#define _circle_sound_wm8960soundcontroller_h

#include <circle/sound/soundcontroller.h>
#include <circle/i2cmaster.h>
#include <circle/types.h>

class CWM8960SoundController : public CSoundController		/// Sound controller for WM8960
{
public:
	CWM8960SoundController (CI2CMaster *pI2CMaster, u8 uchI2CAddress = 0);

	boolean Probe (void) override;

	u32 GetOutputProperties (void) const override
	{
		return PropertyDirectionSupported;
	}

private:
	boolean InitWM8960 (u8 uchI2CAddress);

private:
	CI2CMaster *m_pI2CMaster;
	u8 m_uchI2CAddress;
};

#endif
