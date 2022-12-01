//
// wm8960soundcontroller.cpp
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
#include <circle/sound/wm8960soundcontroller.h>
#include <assert.h>

CWM8960SoundController::CWM8960SoundController (CI2CMaster *pI2CMaster, u8 uchI2CAddress)
:	m_pI2CMaster (pI2CMaster),
	m_uchI2CAddress (uchI2CAddress)
{
}

boolean CWM8960SoundController::Probe (void)
{
	if (m_uchI2CAddress)
	{
		return InitWM8960 (m_uchI2CAddress);
	}

	if (InitWM8960 (0x1A))
	{
		m_uchI2CAddress = 0x1A;

		return TRUE;
	}

	return FALSE;
}

// For WM8960 i2c register is 7 bits and value is 9 bits,
// so let's have a helper for packing this into two bytes
#define SHIFT_BIT(r, v) {((v&0x0100)>>8) | (r<<1), (v&0xff)}

boolean CWM8960SoundController::InitWM8960 (u8 uchI2CAddress)
{
	// based on https://github.com/RASPIAUDIO/ULTRA/blob/main/ultra.c
	// Licensed under GPLv3
	static const u8 initBytes[][2] =
	{
		// reset
		SHIFT_BIT(15, 0x000),
		// Power
		SHIFT_BIT(25, 0x1FC),
		SHIFT_BIT(26, 0x1F9),
		SHIFT_BIT(47, 0x03C),
		// Clock PLL
		SHIFT_BIT(4, 0x001),
		SHIFT_BIT(52, 0x027),
		SHIFT_BIT(53, 0x086),
		SHIFT_BIT(54, 0x0C2),
		SHIFT_BIT(55, 0x026),
		// ADC/DAC
		SHIFT_BIT(5, 0x000),
		SHIFT_BIT(7, 0x002),
		// ALC and Noise control
		SHIFT_BIT(20, 0x0F9),
		SHIFT_BIT(17, 0x1FB),
		SHIFT_BIT(18, 0x000),
		SHIFT_BIT(19, 0x032),
		// OUT1 volume
		SHIFT_BIT(2, 0x16F),
		SHIFT_BIT(3, 0x16F),
		//SPK volume
		SHIFT_BIT(40, 0x17F),
		SHIFT_BIT(41, 0x178),
		SHIFT_BIT(51, 0x08D),
		// input volume
		SHIFT_BIT(0, 0x13F),
		SHIFT_BIT(1, 0x13F),
		// INPUTS
		SHIFT_BIT(32, 0x138),
		SHIFT_BIT(33, 0x138),
		// OUTPUTS
		SHIFT_BIT(49, 0x0F7),
		SHIFT_BIT(10, 0x1FF),
		SHIFT_BIT(11, 0x1FF),
		SHIFT_BIT(34, 0x100),
		SHIFT_BIT(37, 0x100)
	};

	assert (m_pI2CMaster);
	assert (uchI2CAddress);
	for (auto &command : initBytes)
	{
		if (   m_pI2CMaster->Write (uchI2CAddress, &command, sizeof (command))
		    != sizeof (command))
		{
			return FALSE;
		}
	}

	return TRUE;
}
