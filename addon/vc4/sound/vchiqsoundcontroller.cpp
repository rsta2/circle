//
// vchiqsoundcontroller.cpp
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
#include <vc4/sound/vchiqsoundcontroller.h>
#include <vc4/sound/vchiqsoundbasedevice.h>
#include <assert.h>

CVCHIQSoundController::CVCHIQSoundController (CVCHIQSoundBaseDevice *pSoundDevice,
					      TVCHIQSoundDestination Destination)
:	m_pSoundDevice (pSoundDevice),
	m_Destination (Destination),
	m_nOutputVolumedB (VCHIQ_SOUND_VOLUME_DEFAULT / 100)
{
}

boolean CVCHIQSoundController::EnableJack (TJack Jack)
{
	switch (Jack)
	{
	case JackDefaultOut:
		m_Destination = VCHIQSoundDestinationAuto;
		break;

	case JackHeadphone:
		m_Destination = VCHIQSoundDestinationHeadphones;
		break;

	case JackHDMI:
		m_Destination = VCHIQSoundDestinationHDMI;
		break;

	default:
		return FALSE;
	}

	assert (m_pSoundDevice);
	m_pSoundDevice->SetControl (m_nOutputVolumedB * 100, m_Destination);

	return TRUE;
}

const CSoundController::TControlInfo CVCHIQSoundController::GetControlInfo (TControl Control,
									    TJack Jack,
									    TChannel Channel) const
{
	if (IsOutputJack (Jack))
	{
		switch (Control)
		{
		case ControlVolume:
			if (Channel == ChannelAll)
			{
				return { TRUE, VCHIQ_SOUND_VOLUME_MIN / 100,
					       VCHIQ_SOUND_VOLUME_MAX / 100 };
			}
			break;

		case ControlMute:
		default:
			break;
		}
	}

	return { FALSE };
}

boolean CVCHIQSoundController::SetControl (TControl Control, TJack Jack, TChannel Channel,
					   int nValue)
{
	switch (Control)
	{
	case ControlVolume:
		return SetVolume (Jack, Channel, nValue);

	case ControlMute:
	default:
		break;
	}

	return FALSE;
}

boolean CVCHIQSoundController::SetVolume (TJack Jack, TChannel Channel, int ndB)
{
	if (   !IsOutputJack (Jack)
	    || Channel != ChannelAll)
	{
		return FALSE;
	}

	m_nOutputVolumedB = ndB;

	assert (m_pSoundDevice);
	m_pSoundDevice->SetControl (m_nOutputVolumedB * 100, m_Destination);

	return TRUE;
}
