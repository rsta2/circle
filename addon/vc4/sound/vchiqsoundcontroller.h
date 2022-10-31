//
// vchiqsoundcontroller.h
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
#ifndef _vc4_sound_vchiqsoundcontroller_h
#define _vc4_sound_vchiqsoundcontroller_h

#include <circle/sound/soundcontroller.h>

#define VCHIQ_SOUND_VOLUME_MIN		-10000
#define VCHIQ_SOUND_VOLUME_DEFAULT	0
#define VCHIQ_SOUND_VOLUME_MAX		400

enum TVCHIQSoundDestination
{
	VCHIQSoundDestinationAuto,
	VCHIQSoundDestinationHeadphones,
	VCHIQSoundDestinationHDMI,
	VCHIQSoundDestinationUnknown
};

class CVCHIQSoundBaseDevice;

class CVCHIQSoundController : public CSoundController
{
public:
	CVCHIQSoundController (CVCHIQSoundBaseDevice *pSoundDevice,
			       TVCHIQSoundDestination Destination);

	u32 GetOutputProperties (void) const override
	{
		return PropertyDirectionSupported;
	}

	boolean EnableJack (TJack Jack) override;


	const TControlInfo GetControlInfo (TControl Control, TJack Jack,
					   TChannel Channel) const override;
	boolean SetControl (TControl Control, TJack Jack, TChannel Channel, int nValue) override;

private:
	boolean SetVolume (TJack Jack, TChannel Channel, int ndB);

private:
	CVCHIQSoundBaseDevice *m_pSoundDevice;

	TVCHIQSoundDestination m_Destination;
	int m_nOutputVolumedB;
};

#endif
