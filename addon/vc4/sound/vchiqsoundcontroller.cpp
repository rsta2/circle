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
	m_nOutputVolumedB (VCHIQ_SOUND_VOLUME_DEFAULT)
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
	m_pSoundDevice->SetControl (m_nOutputVolumedB, m_Destination);

	return TRUE;
}

boolean CVCHIQSoundController::SetVolume (TJack Jack, int ndB, TChannel Channel)
{
	if (   !IsOutputJack (Jack)
	    || Channel != ChannelAll)
	{
		return FALSE;
	}

	m_nOutputVolumedB = ndB;

	assert (m_pSoundDevice);
	m_pSoundDevice->SetControl (m_nOutputVolumedB, m_Destination);

	return TRUE;
}

const CSoundController::TRange CVCHIQSoundController::GetVolumeRange (TJack Jack) const
{
	return { VCHIQ_SOUND_VOLUME_MIN, VCHIQ_SOUND_VOLUME_MAX };
}
