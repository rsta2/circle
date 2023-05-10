//
// pcm512xsoundcontroller.cpp
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
#include <circle/sound/pcm512xsoundcontroller.h>
#include <assert.h>

CPCM512xSoundController::CPCM512xSoundController (CI2CMaster *pI2CMaster, u8 uchI2CAddress)
:	m_pI2CMaster (pI2CMaster),
	m_uchI2CAddress (uchI2CAddress),
	m_uchMuteValue (0)
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


const CSoundController::TControlInfo CPCM512xSoundController::GetControlInfo (TControl Control,
									      TJack Jack,
									      TChannel Channel) const
{
	if (IsOutputJack (Jack))
	{
		switch (Control)
		{
		case ControlMute:
			return { TRUE, 0, 1 };

		case ControlVolume:
			return { TRUE, -103, 12 };

		default:
			break;
		}
	}

	return { FALSE };
}

boolean CPCM512xSoundController::SetControl (TControl Control, TJack Jack, TChannel Channel,
					     int nValue)
{
	switch (Control)
	{
	case ControlMute:
		return SetMute (Jack, Channel, !!nValue);

	case ControlVolume:
		return SetVolume (Jack, Channel, nValue);

	default:
		break;
	}

	return FALSE;
}

boolean CPCM512xSoundController::SetMute (TJack Jack, TChannel Channel, boolean bEnable)
{
	assert (m_pI2CMaster);
	assert (m_uchI2CAddress);

	if (!IsOutputJack (Jack))
	{
		return FALSE;
	}

	u8 uchMask = 0;
	switch (Channel)
	{
	case ChannelAll:	uchMask = 0x11;		break;
	case ChannelLeft:	uchMask = 0x10;		break;
	case ChannelRight:	uchMask = 0x01;		break;

	default:
		return FALSE;
	}

	if (bEnable)
	{
		m_uchMuteValue |= uchMask;
	}
	else
	{
		m_uchMuteValue &= ~uchMask;
	}

	const u8 Cmd[] = {3, m_uchMuteValue};
	return m_pI2CMaster->Write (m_uchI2CAddress, Cmd, sizeof Cmd) >= 0;
}

boolean CPCM512xSoundController::SetVolume (TJack Jack, TChannel Channel, int ndB)
{
	assert (m_pI2CMaster);
	assert (m_uchI2CAddress);

	if (!IsOutputJack (Jack))
	{
		return FALSE;
	}

	if (Channel > ChannelRight)
	{
		return FALSE;
	}

	if (ndB < -103)
	{
		ndB = -103;
	}
	else if (ndB > 12)
	{
		ndB = 12;
	}

	u8 uchVolume = (u8) (0b110000 - (ndB * 2));

	if (   Channel == ChannelLeft
	    || Channel == ChannelAll)
	{
		const u8 Cmd[] = {61, uchVolume};
		if (m_pI2CMaster->Write (m_uchI2CAddress, Cmd, sizeof Cmd) < 0)
		{
			return FALSE;
		}
	}

	if (   Channel == ChannelRight
	    || Channel == ChannelAll)
	{
		const u8 Cmd[] = {62, uchVolume};
		if (m_pI2CMaster->Write (m_uchI2CAddress, Cmd, sizeof Cmd) < 0)
		{
			return FALSE;
		}
	}

	return TRUE;
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
