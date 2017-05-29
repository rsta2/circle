//
// pwmsound.cpp
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
#include "kernel.h"
#include "pwmsound.h"
#include <assert.h>

// Playback works with 44100 Hz, Stereo, 12 Bit only.
// Other sound formats will be converted on the fly.

#define SAMPLE_RATE		44100

void DSPCallBack(void* unused, unsigned char *stream, int len);

CPWMSound::CPWMSound(CInterruptSystem *pInterrupt)
:	CPWMSoundBaseDevice (pInterrupt, SAMPLE_RATE),
	m_pSoundData (0)
{
	assert ((1 << 12) <= GetRange () && GetRange () < (1 << 13));	// 12 bit range
}

CPWMSound::~CPWMSound(void)
{
}
 
void CPWMSound::Playback (void *pSoundData, unsigned nSamples, unsigned nChannels, unsigned nBitsPerSample)
{
	assert (!IsActive ());

	assert (pSoundData != 0);
	assert (nChannels == 1 || nChannels == 2);
	assert (nBitsPerSample == 8 || nBitsPerSample == 16);

	m_p = m_pSoundData	 = (u8 *) pSoundData;
	m_s = m_nSamples	 = nSamples;
	m_nChannels	 = nChannels;
	m_nBitsPerSample = nBitsPerSample;

	Start ();
}

boolean CPWMSound::PlaybackActive (void) const
{
	return IsActive ();
}

void CPWMSound::CancelPlayback (void)
{
	Cancel ();
}

unsigned CPWMSound::GetChunk (u32 *pBuffer, unsigned nChunkSize)
{
	assert (pBuffer != 0);
	assert (nChunkSize > 0);
	assert ((nChunkSize & 1) == 0);

	unsigned nResult = 0;

	if (1)
	{
		m_kernel->dspcallback(pBuffer, nChunkSize);
		return nChunkSize;
		//m_pSoundData	 = (u8 *) m_p;
		//m_nSamples	 = m_s;
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
		
		if (m_nBitsPerSample >= 12)
		{
			nValue >>= m_nBitsPerSample - 12;
		}
		else
		{
			nValue <<= 12 - m_nBitsPerSample;
		}

		pBuffer[nSample++] = nValue;
	
		if (m_nChannels == 2)
		{
			nValue = *m_pSoundData++;
			if (m_nBitsPerSample > 8)
			{
				nValue |= (unsigned) *m_pSoundData++ << 8;
				nValue = (nValue + 0x8000) & 0xFFFF;	// signed -> unsigned (16 bit)
			}

			if (m_nBitsPerSample >= 12)
			{
				nValue >>= m_nBitsPerSample - 12;
			}
			else
			{
				nValue <<= 12 - m_nBitsPerSample;
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
