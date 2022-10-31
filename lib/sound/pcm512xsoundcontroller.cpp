//
// pcm512xsoundcontroller.cpp
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
#include <circle/sound/pcm512xsoundcontroller.h>
#include <assert.h>

CPCM512xSoundController::CPCM512xSoundController (CI2CMaster *pI2CMaster, u8 uchI2CAddress)
:	m_pI2CMaster (pI2CMaster),
	m_uchI2CAddress (uchI2CAddress)
{
}

boolean CPCM512xSoundController::Probe (void)
{
	if (m_uchI2CAddress)
	{
		return InitPCM512x (m_uchI2CAddress);
	}

	if (InitPCM512x (0x4C))
	{
		m_uchI2CAddress = 0x4C;

		return TRUE;
	}

	if (InitPCM512x (0x4D))
	{
		m_uchI2CAddress = 0x4D;

		return TRUE;
	}

	return FALSE;
}

boolean CPCM512xSoundController::SetMute (TJack Jack, boolean bEnable)
{
	assert (m_pI2CMaster);
	assert (m_uchI2CAddress);

	if (!IsOutputJack (Jack))
	{
		return FALSE;
	}

	const u8 Cmd[] = {3, (u8) (bEnable ? 0x11 : 0)};
	return m_pI2CMaster->Write (m_uchI2CAddress, Cmd, sizeof Cmd) >= 0;
}

boolean CPCM512xSoundController::SetVolume (TJack Jack, int ndB, TChannel Channel)
{
	assert (m_pI2CMaster);
	assert (m_uchI2CAddress);

	if (!IsOutputJack (Jack))
	{
		return FALSE;
	}

	if (ndB < -103)
	{
		ndB = -103;
	}
	else if (ndB > 24)
	{
		ndB = 24;
	}

	u8 uchVolume = (u8) (0b110000 - (ndB * 2));

	if (Channel & ChannelLeft)
	{
		const u8 Cmd[] = {61, uchVolume};
		if (m_pI2CMaster->Write (m_uchI2CAddress, Cmd, sizeof Cmd) < 0)
		{
			return FALSE;
		}
	}

	if (Channel & ChannelRight)
	{
		const u8 Cmd[] = {62, uchVolume};
		if (m_pI2CMaster->Write (m_uchI2CAddress, Cmd, sizeof Cmd) < 0)
		{
			return FALSE;
		}
	}

	return TRUE;
}

const CSoundController::TRange CPCM512xSoundController::GetVolumeRange (TJack Jack) const
{
	return { -103, 24 };
}

//
// Taken from the file mt32pi.cpp from this project:
//
// mt32-pi - A baremetal MIDI synthesizer for Raspberry Pi
// Copyright (C) 2020-2021 Dale Whinham <daleyo@gmail.com>
//
// Licensed under GPLv3
//
boolean CPCM512xSoundController::InitPCM512x (u8 uchI2CAddress)
{
	static const u8 initBytes[][2] =
	{
		// Set PLL reference clock to BCK (set SREF to 001b)
		{ 0x0d, 0x10 },

		// Ignore clock halt detection (set IDCH to 1)
		{ 0x25, 0x08 },

		// Disable auto mute
		{ 0x41, 0x04 }
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
