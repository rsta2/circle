//
// vchiqsounddevice.cpp
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
#include <vc4/sound/vchiqsounddevice.h>
#include <assert.h>

// Playback works with 44100 Hz, Stereo, 16 Bit only.
// Other sound formats will be converted on the fly.

#define SAMPLE_RATE		44100

#define CHUNK_SIZE		4000

CVCHIQSoundDevice::CVCHIQSoundDevice (CVCHIQDevice *pVCHIQDevice, TVCHIQSoundDestination Destination)
:	CVCHIQSoundBaseDevice (pVCHIQDevice, SAMPLE_RATE, CHUNK_SIZE, Destination),
	m_pSoundData (0)
{
}

CVCHIQSoundDevice::~CVCHIQSoundDevice (void)
{
}

boolean CVCHIQSoundDevice::Playback (void     *pSoundData,
				     unsigned  nSamples,
				     unsigned  nChannels,
				     unsigned  nBitsPerSample)
{
	assert (!IsActive ());

	assert (pSoundData != 0);
	assert (nChannels == 1 || nChannels == 2);
	assert (nBitsPerSample == 8 || nBitsPerSample == 16);

	m_pSoundData	 = (u8 *) pSoundData;
	m_nSamples	 = nSamples;
	m_nChannels	 = nChannels;
	m_nBitsPerSample = nBitsPerSample;

	return Start ();
}

boolean CVCHIQSoundDevice::PlaybackActive (void) const
{
	return IsActive ();
}

void CVCHIQSoundDevice::CancelPlayback (void)
{
	Cancel ();
}

void CVCHIQSoundDevice::SetControl (int nVolume, TVCHIQSoundDestination Destination)
{
	CVCHIQSoundBaseDevice::SetControl (nVolume, Destination);
}

unsigned CVCHIQSoundDevice::GetChunk (s16 *pBuffer, unsigned nChunkSize)
{
	assert (pBuffer != 0);
	assert (nChunkSize > 0);
	assert ((nChunkSize & 1) == 0);

	unsigned nResult = 0;

	if (m_nSamples == 0)
	{
		return nResult;
	}

	assert (m_pSoundData != 0);
	assert (m_nChannels == 1 || m_nChannels == 2);
	assert (m_nBitsPerSample == 8 || m_nBitsPerSample == 16);

	for (unsigned nSample = 0; nSample < nChunkSize / 2;)		// 2 channels on output
	{
		unsigned nValue = *m_pSoundData++;
		if (m_nBitsPerSample > 8)
		{
			nValue |= (unsigned) *m_pSoundData++ << 8;
		}
		else
		{
			nValue -= 128;			// unsigned (8 bit) -> signed (16 bit)
			nValue *= 256;
		}

		pBuffer[nSample++] = nValue;

		if (m_nChannels == 2)
		{
			nValue = *m_pSoundData++;
			if (m_nBitsPerSample > 8)
			{
				nValue |= (unsigned) *m_pSoundData++ << 8;
			}
			else
			{
				nValue -= 128;		// unsigned (8 bit) -> signed (16 bit)
				nValue *= 256;
			}
		}

		pBuffer[nSample++] = nValue;

		nResult += 2;

		if (--m_nSamples == 0)
		{
			break;
		}
	}

	return nResult;
}
