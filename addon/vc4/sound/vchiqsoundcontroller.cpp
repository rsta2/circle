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

void CVCHIQSoundController::SelectOutput (TOutputSelector Selector)
{
	switch (Selector)
	{
	case OutputSelectorDefault:
		m_Destination = VCHIQSoundDestinationAuto;
		break;

	case OutputSelectorHeadphones:
		m_Destination = VCHIQSoundDestinationHeadphones;
		break;

	case OutputSelectorHDMI:
		m_Destination = VCHIQSoundDestinationHDMI;
		break;

	default:
		return;
	}

	assert (m_pSoundDevice);
	m_pSoundDevice->SetControl (m_nOutputVolumedB, m_Destination);
}

void CVCHIQSoundController::SetOutputVolume (int ndB)
{
	m_nOutputVolumedB = ndB;

	assert (m_pSoundDevice);
	m_pSoundDevice->SetControl (m_nOutputVolumedB, m_Destination);
}

const CSoundController::TRange CVCHIQSoundController::GetOutputVolumeRange (void) const
{
	return { VCHIQ_SOUND_VOLUME_MIN, VCHIQ_SOUND_VOLUME_MAX };
}
