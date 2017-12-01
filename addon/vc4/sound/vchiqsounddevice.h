//
// vchiqsounddevice.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2017  R. Stange <rsta2@o2online.de>
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
#ifndef _vc4_sound_vchiqsounddevice_h
#define _vc4_sound_vchiqsounddevice_h

#include <vc4/sound/vchiqsoundbasedevice.h>
#include <vc4/vchiq/vchiqdevice.h>
#include <circle/types.h>

class CVCHIQSoundDevice : private CVCHIQSoundBaseDevice	/// Higher level access to VCHIQ sound service
{
public:
	/// \param pVCHIQDevice	pointer to the VCHIQ interface device
	/// \param Destination	the target device, the sound data is sent to\n
	///			(detected automatically, if equal to VCHIQSoundDestinationAuto)
	CVCHIQSoundDevice (CVCHIQDevice *pVCHIQDevice,
			   TVCHIQSoundDestination Destination = VCHIQSoundDestinationAuto);

	~CVCHIQSoundDevice (void);

	/// \brief Starts playback of a sound clip (at a sample rate of 44100 Hz)
	/// \param pSoundData	   pointer to the sound data
	/// \param nSamples	   number of samples (for Stereo the L/R samples are count as one)
	/// \param nChannels	   1 (Mono) or 2 (Stereo)
	/// \param nBitsPerSample  8 (unsigned sound data) or 16 (signed sound data)
	/// \return Operation successful?
	boolean Playback (void	   *pSoundData,
			  unsigned  nSamples,
			  unsigned  nChannels,
			  unsigned  nBitsPerSample);

	/// \return Is playback running?
	boolean PlaybackActive (void) const;

	/// \brief Stops playback
	/// \note Cancel takes effect after a short delay
	void CancelPlayback (void);

	/// \param nVolume	Output volume to be set (-10000..400)
	/// \param Destination	the target device, the sound data is sent to\n
	///			(not modified, if equal to VCHIQSoundDestinationUnknown)
	/// \note This method can be called, while playback is running.
	void SetControl (int nVolume,
			 TVCHIQSoundDestination Destination = VCHIQSoundDestinationUnknown);

private:
	virtual unsigned GetChunk (s16 *pBuffer, unsigned  nChunkSize);

private:
	u8	 *m_pSoundData;
	unsigned  m_nSamples;
	unsigned  m_nChannels;
	unsigned  m_nBitsPerSample;
};

#endif
