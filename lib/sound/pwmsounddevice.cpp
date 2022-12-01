//
// pwmsounddevice.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2022  R. Stange <rsta2@o2online.de>
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
#include <circle/sound/pwmsounddevice.h>
#include <assert.h>

// Playback works with 44100 Hz, Stereo only.
// Other sound formats will be converted on the fly.

#define SAMPLE_RATE		44100

CPWMSoundDevice::CPWMSoundDevice (CInterruptSystem *pInterrupt)
:	CPWMSoundBaseDevice (pInterrupt, SAMPLE_RATE),
	m_nRangeBits (0),
	m_bChannelsSwapped (AreChannelsSwapped ()),
	m_pSoundData (0)
{
	assert (GetRangeMin () == 0);

	for (unsigned nBits = 2; nBits < 16; nBits++)
	{
		if (GetRangeMax () < (1 << nBits))
		{
			m_nRangeBits = nBits-1;

			break;
		}
	}

	assert (m_nRangeBits > 0);
}

CPWMSoundDevice::~CPWMSoundDevice (void)
{
}

void CPWMSoundDevice::Playback (void *pSoundData, unsigned nSamples, unsigned nChannels, unsigned nBitsPerSample)
{
	assert (!IsActive ());

	assert (pSoundData != 0);
	assert (nChannels == 1 || nChannels == 2);
	assert (nBitsPerSample == 8 || nBitsPerSample == 16);

	m_pSoundData	 = (u8 *) pSoundData;
	m_nSamples	 = nSamples;
	m_nChannels	 = nChannels;
	m_nBitsPerSample = nBitsPerSample;

	Start ();
}

boolean CPWMSoundDevice::PlaybackActive (void) const
{
	return IsActive ();
}

void CPWMSoundDevice::CancelPlayback (void)
{
	Cancel ();
}

unsigned CPWMSoundDevice::GetChunk (u32 *pBuffer, unsigned nChunkSize)
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
			nValue = (nValue + 0x8000) & 0xFFFF;		// signed -> unsigned (16 bit)
		}
		
		if (m_nBitsPerSample >= m_nRangeBits)
		{
			nValue >>= m_nBitsPerSample - m_nRangeBits;
		}
		else
		{
			nValue <<= m_nRangeBits - m_nBitsPerSample;
		}

		unsigned nValue2 = nValue;
		if (m_nChannels == 2)
		{
			nValue2 = *m_pSoundData++;
			if (m_nBitsPerSample > 8)
			{
				nValue2 |= (unsigned) *m_pSoundData++ << 8;
				nValue2 = (nValue2 + 0x8000) & 0xFFFF;	// signed -> unsigned (16 bit)
			}

			if (m_nBitsPerSample >= m_nRangeBits)
			{
				nValue2 >>= m_nBitsPerSample - m_nRangeBits;
			}
			else
			{
				nValue2 <<= m_nRangeBits - m_nBitsPerSample;
			}
		}

		if (!m_bChannelsSwapped)
		{
			pBuffer[nSample++] = nValue;
			pBuffer[nSample++] = nValue2;
		}
		else
		{
			pBuffer[nSample++] = nValue2;
			pBuffer[nSample++] = nValue;
		}

		nResult += 2;

		if (--m_nSamples == 0)
		{
			break;
		}
	}

	return nResult;
}
