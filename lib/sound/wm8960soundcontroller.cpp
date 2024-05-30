//
// wm8960soundcontroller.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2022-2024  R. Stange <rsta2@o2online.de>
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

CWM8960SoundController::CWM8960SoundController (CI2CMaster *pI2CMaster, u8 uchI2CAddress,
						unsigned nSampleRate,
						boolean bOutSupported, boolean bInSupported)
:	m_pI2CMaster (pI2CMaster),
	m_uchI2CAddress (uchI2CAddress ? uchI2CAddress : 0x1A),
	m_nSampleRate (nSampleRate),
	m_bOutSupported (bOutSupported),
	m_bInSupported (bInSupported),
	m_uchInVolume {0x17, 0x17}		// 0 dB
{
	assert (m_bOutSupported || m_bInSupported);
}

boolean CWM8960SoundController::Probe (void)
{
	// Originally based on https://github.com/RASPIAUDIO/ULTRA/blob/main/ultra.c
	// Licensed under GPLv3

	// Reset
	if (!WriteReg (15, 0x000))
	{
		return FALSE;
	}

	// Power Management
	if (   !WriteReg (25, m_bInSupported ? 0x1FC : 0x1C0)
	    || !WriteReg (26, m_bOutSupported ? 0x1F9 : 0x001)
	    || !WriteReg (47,   (m_bInSupported ? 0x030 : 0x000)
			      | (m_bOutSupported ? 0x00C : 0x000)))
	{
		return FALSE;
	}

	// Clocking / PLL
	if (m_nSampleRate == 44100)
	{
		if (   !WriteReg (4, 0x005)
		    || !WriteReg (52, 0x037)
		    || !WriteReg (53, 0x086)
		    || !WriteReg (54, 0x0C2)
		    || !WriteReg (55, 0x026))
		{
			return FALSE;
		}
	}
	else if (m_nSampleRate == 48000)
	{
		if (   !WriteReg (4, 0x005)
		    || !WriteReg (52, 0x038)
		    || !WriteReg (53, 0x031)
		    || !WriteReg (54, 0x026)
		    || !WriteReg (55, 0x0E8))
		{
			return FALSE;
		}
	}
	else
	{
		return FALSE;
	}

	if (   !WriteReg (5, 0x000)	// ADC and DAC Control
	    || !WriteReg (7, 0x00A)	// Audio Interface
	    || !WriteReg (20, 0x0F9))	// Noise control
	{
		return FALSE;
	}

	// Volume
	if (   !WriteReg (2, 0x179)	// OUT1 L (Headphone) 0 dB
	    || !WriteReg (3, 0x179)	// OUT1 R (Headphone) 0 dB
	    || !WriteReg (40, 0x179)	// Speaker L 0 dB
	    || !WriteReg (41, 0x179)	// Speaker R 0 dB
	    || !WriteReg (51, 0x08D)	// Speaker Boost
	    || !WriteReg (0, 0x100 | m_uchInVolume[0])	// Input L
	    || !WriteReg (1, 0x100 | m_uchInVolume[1]))	// Input R
	{
		return FALSE;
	}

	// Inputs / Outputs
	if (   !WriteReg (32, 0x138)	// ADCL Signal Path
	    || !WriteReg (33, 0x138)	// ADCR Signal Path
	    || !WriteReg (49, 0x0F7)	// Speaker Control
	    || !WriteReg (34, 0x100)	// Left Out Mix
	    || !WriteReg (37, 0x100))	// Right Out Mix
	{
		return FALSE;
	}

	return TRUE;
}

boolean CWM8960SoundController::EnableJack (TJack Jack)
{
	switch (Jack)
	{
	case JackSpeaker:
		return WriteReg (49, 0x0F7);	// Speaker Control

	case JackDefaultOut:
	case JackLineOut:
	case JackHeadphone:
	case JackDefaultIn:
	case JackMicrophone:
		return TRUE;

	default:
		break;
	}

	return FALSE;
}

// Headphone jack cannot be disabled
boolean CWM8960SoundController::DisableJack (TJack Jack)
{
	if (Jack == JackSpeaker)
	{
		return WriteReg (49, 0x037);	// Speaker Control
	}

	return FALSE;
}

const CSoundController::TControlInfo CWM8960SoundController::GetControlInfo (TControl Control,
									     TJack Jack,
									     TChannel Channel) const
{
	switch (Control)
	{
	case ControlMute:
		if (IsInputJack (Jack))
		{
			return { TRUE, 0, 1 };
		}
		break;

	case ControlVolume:
		if (IsInputJack (Jack))
		{
			return { TRUE, -17, 30 };
		}
		else
		{
			return { TRUE, -73, 6 };
		}
		break;

	case ControlALC:
		if (   IsInputJack (Jack)
		    && Channel == ChannelAll)
		{
			return { TRUE, 0, 1 };
		}
		break;

	default:
		break;
	}

	return { FALSE, 0, 0 };
}

boolean CWM8960SoundController::SetControl (TControl Control, TJack Jack, TChannel Channel, int nValue)
{
	switch (Control)
	{
	case ControlMute:
		if (IsInputJack (Jack))
		{
			if (   Channel == ChannelLeft
			    || Channel == ChannelAll)
			{
				m_uchInVolume[0] &= 0x3F;
				m_uchInVolume[0] |= nValue ? 0x80 : 0x00;

				if (!WriteReg (0, 0x100 | m_uchInVolume[0]))
				{
					return FALSE;
				}
			}

			if (   Channel == ChannelRight
			    || Channel == ChannelAll)
			{
				m_uchInVolume[1] &= 0x3F;
				m_uchInVolume[1] |= nValue ? 0x80 : 0x00;

				if (!WriteReg (1, 0x100 | m_uchInVolume[1]))
				{
					return FALSE;
				}
			}

			return TRUE;
		}
		break;

	case ControlVolume:
		if (IsInputJack (Jack))
		{
			if (!(-17 <= nValue && nValue <= 30))
			{
				return FALSE;
			}

			u8 uchValue = (nValue + 17) * 100 / 75;

			if (   Channel == ChannelLeft
			    || Channel == ChannelAll)
			{
				m_uchInVolume[0] &= 0x80;
				m_uchInVolume[0] |= uchValue;

				if (!WriteReg (0, 0x100 | m_uchInVolume[0]))
				{
					return FALSE;
				}
			}

			if (   Channel == ChannelRight
			    || Channel == ChannelAll)
			{
				m_uchInVolume[1] &= 0x80;
				m_uchInVolume[1] |= uchValue;

				if (!WriteReg (1, 0x100 | m_uchInVolume[1]))
				{
					return FALSE;
				}
			}

			return TRUE;
		}
		else
		{
			if (!(-73 <= nValue && nValue <= 6))
			{
				return FALSE;
			}

			u8 uchValue = nValue + 73 + 0x30;

			u8 nRegL = Jack == JackSpeaker ? 40 : 2;

			if (   Channel == ChannelLeft
			    || Channel == ChannelAll)
			{
				if (!WriteReg (nRegL, 0x100 | uchValue))
				{
					return FALSE;
				}
			}

			if (   Channel == ChannelRight
			    || Channel == ChannelAll)
			{
				if (!WriteReg (nRegL + 1, 0x100 | uchValue))
				{
					return FALSE;
				}
			}

			return TRUE;
		}
		break;

	case ControlALC:
		if (   IsInputJack (Jack)
		    && Channel == ChannelAll)
		{
			if (nValue)
			{
				// Ensure that left and right input volumes are the same
				m_uchInVolume[1] = m_uchInVolume[0];
				if (!WriteReg (1, 0x100 | m_uchInVolume[1]))
				{
					return FALSE;
				}

				return WriteReg (17, 0x1FB);
			}
			else
			{
				return WriteReg (17, 0x00B);
			}
		}
		break;

	default:
		break;
	}

	return FALSE;
}

boolean CWM8960SoundController::WriteReg (u8 nReg, u16 nValue)
{
	assert (nReg <= 0x7F);
	assert (nValue <= 0x1FF);
	const u8 Cmd[] = { (u8) ((nReg << 1) | (nValue >> 8)), (u8) nValue };

	assert (m_pI2CMaster);
	assert (m_uchI2CAddress);
	return m_pI2CMaster->Write (m_uchI2CAddress, Cmd, sizeof Cmd) == sizeof Cmd;
}
