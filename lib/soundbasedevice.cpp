//
// soundbasedevice.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2021  R. Stange <rsta2@o2online.de>
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
#include <circle/soundbasedevice.h>
#include <circle/macros.h>
#include <circle/util.h>
#include <assert.h>

CSoundBaseDevice::CSoundBaseDevice (TSoundFormat HWFormat, u32 nRange32, unsigned nSampleRate,
				    boolean bSwapChannels)
:	m_HWFormat (HWFormat),
	m_nSampleRate (nSampleRate),
	m_bSwapChannels (bSwapChannels),
	m_nQueueSize (0),
	m_nNeedDataThreshold (0),
	m_WriteFormat (SoundFormatUnknown),
	m_nWriteChannels (0),
	m_pQueue (0),
	m_nInPtr (0),
	m_nOutPtr (0),
	m_pCallback (0)
{
	memset (m_NullFrame, 0, sizeof m_NullFrame);

	switch (m_HWFormat)
	{
	case SoundFormatSigned16:
		m_nHWSampleSize = sizeof (s16);
		m_nRangeMin = -32768;
		m_nRangeMax = 32767;
		break;

	case SoundFormatSigned24:
	case SoundFormatIEC958:
		m_nHWSampleSize = sizeof (u32);
		m_nRangeMin = -(1 << 23)+1;
		m_nRangeMax = (1 << 23)-1;
		break;

	case SoundFormatUnsigned32: {
		m_nHWSampleSize = sizeof (u32);
		m_nRangeMin = 0;
		m_nRangeMax = (int) (nRange32-1);
		assert (m_nRangeMax > 0);

		u32 *pSample32 = reinterpret_cast<u32 *> (m_NullFrame);
		pSample32[0] = pSample32[1] = m_nRangeMax / 2;
		} break;

	default:
		assert (0);
		break;
	}

	m_nHWFrameSize = SOUND_HW_CHANNELS * m_nHWSampleSize;

	if (m_HWFormat == SoundFormatIEC958)
	{
		u8 uchFS = 0, uchOrigFS = 0;
		switch (m_nSampleRate)
		{
		case 22050:	uchFS = 4;	uchOrigFS = 11;	break;
		case 24000:	uchFS = 6;	uchOrigFS = 9;	break;
		case 32000:	uchFS = 3;	uchOrigFS = 12;	break;
		case 44100:	uchFS = 0;	uchOrigFS = 15;	break;
		case 48000:	uchFS = 2;	uchOrigFS = 13;	break;
		case 88200:	uchFS = 8;	uchOrigFS = 7;	break;
		case 96000:	uchFS = 10;	uchOrigFS = 5;	break;
		case 176400:	uchFS = 12;	uchOrigFS = 3;	break;
		case 192000:	uchFS = 14;	uchOrigFS = 1;	break;

		default:
			assert (0);
			break;
		}

		m_uchIEC958Status[0] = 0b100;	// consumer, PCM, no copyright, no pre-emphasis
		m_uchIEC958Status[1] = 0;	// category (general mode)
		m_uchIEC958Status[2] = 0;	// source number, take no account of channel number
		m_uchIEC958Status[3] = uchFS;	// sampling frequency
		m_uchIEC958Status[4] = 0b1011 | (uchOrigFS << 4); // 24 bit samples, original freq.
	}
}

CSoundBaseDevice::~CSoundBaseDevice (void)
{
	m_pCallback = 0;

	delete [] m_pQueue;
	m_pQueue = 0;
}

int CSoundBaseDevice::GetRangeMin (void) const
{
	return m_nRangeMin;
}

int CSoundBaseDevice::GetRangeMax (void) const
{
	return m_nRangeMax;
}

boolean CSoundBaseDevice::AllocateQueue (unsigned nSizeMsecs)
{
	assert (m_pQueue == 0);
	assert (1 <= nSizeMsecs && nSizeMsecs <= 1000);

	// 1 byte remains free
	m_nQueueSize = (m_nHWFrameSize*m_nSampleRate*nSizeMsecs + 999) / 1000 + 1;

	m_pQueue = new u8[m_nQueueSize];
	if (m_pQueue == 0)
	{
		return FALSE;
	}

	m_nNeedDataThreshold = m_nQueueSize / 2;

	return TRUE;
}

boolean CSoundBaseDevice::AllocateQueueFrames (unsigned nSizeFrames)
{
	assert (m_pQueue == 0);
	assert (1 <= nSizeFrames && nSizeFrames <= m_nSampleRate);

	// 1 byte remains free
	m_nQueueSize = m_nHWFrameSize*nSizeFrames + 1;

	m_pQueue = new u8[m_nQueueSize];
	if (m_pQueue == 0)
	{
		return FALSE;
	}

	m_nNeedDataThreshold = m_nQueueSize / 2;

	return TRUE;
}

void CSoundBaseDevice::SetWriteFormat (TSoundFormat Format, unsigned nChannels)
{
	assert (Format < SoundFormatUnknown);
	m_WriteFormat = Format;

	assert (1 <= nChannels && nChannels <= 2);
	m_nWriteChannels = nChannels;

	switch (m_WriteFormat)
	{
	case SoundFormatUnsigned8:
		m_nWriteSampleSize = sizeof (u8);
		break;

	case SoundFormatSigned16:
		m_nWriteSampleSize = sizeof (s16);
		break;

	case SoundFormatSigned24:
		m_nWriteSampleSize = sizeof (u8)*3;
		break;

	default:
		assert (0);
		break;
	}

	m_nWriteFrameSize = m_nWriteChannels * m_nWriteSampleSize;
}

int CSoundBaseDevice::Write (const void *pBuffer, size_t nCount)
{
	assert (m_WriteFormat < SoundFormatUnknown);

	assert (pBuffer != 0);
	const u8 *pBuffer8 = static_cast<const u8 *> (pBuffer);

	int nResult = 0;

	m_SpinLock.Acquire ();

	if (   m_HWFormat == m_WriteFormat
	    && m_nWriteChannels == SOUND_HW_CHANNELS
	    && !m_bSwapChannels)
	{
		// fast path for Stereo samples without bit depth conversion or channel swapping

		unsigned nBytes = GetQueueBytesFree ();
		if (nBytes > nCount)
		{
			nBytes = nCount;
		}
		nBytes &= ~(m_nWriteFrameSize-1);	// must be a multiple of frame size

		if (nBytes > 0)
		{
			Enqueue (pBuffer, nBytes);

			nResult = nBytes;
		}
	}
	else
	{
		while (   nCount >= m_nWriteFrameSize
		       && GetQueueBytesFree () >= m_nHWFrameSize)
		{
			u8 Frame[SOUND_MAX_FRAME_SIZE];

			if (!m_bSwapChannels)
			{
				ConvertSoundFormat (Frame, pBuffer8);
				pBuffer8 += m_nWriteSampleSize;

				if (m_nWriteChannels == 2)
				{
					ConvertSoundFormat (Frame+m_nHWSampleSize, pBuffer8);
					pBuffer8 += m_nWriteSampleSize;
				}
				else
				{
					memcpy (Frame+m_nHWSampleSize, Frame, m_nHWSampleSize);
				}
			}
			else
			{
				ConvertSoundFormat (Frame+m_nHWSampleSize, pBuffer8);
				pBuffer8 += m_nWriteSampleSize;

				if (m_nWriteChannels == 2)
				{
					ConvertSoundFormat (Frame, pBuffer8);
					pBuffer8 += m_nWriteSampleSize;
				}
				else
				{
					memcpy (Frame, Frame+m_nHWSampleSize, m_nHWSampleSize);
				}
			}

			Enqueue (Frame, m_nHWFrameSize);

			nCount -= m_nWriteFrameSize;
			nResult += m_nWriteFrameSize;
		}
	}

	m_SpinLock.Release ();

	return nResult;
}

unsigned CSoundBaseDevice::GetQueueSizeFrames (void)
{
	assert (m_nQueueSize > 0);
	return m_nQueueSize / m_nHWFrameSize;
}

unsigned CSoundBaseDevice::GetQueueFramesAvail (void)
{
	assert (m_nQueueSize > 0);

	m_SpinLock.Acquire ();

	unsigned nQueueBytesAvail = GetQueueBytesAvail ();

	m_SpinLock.Release ();

	return nQueueBytesAvail / m_nHWFrameSize;
}

void CSoundBaseDevice::RegisterNeedDataCallback (TSoundNeedDataCallback *pCallback, void *pParam)
{
	assert (m_pCallback == 0);
	m_pCallback = pCallback;
	assert (m_pCallback != 0);

	m_pCallbackParam = pParam;
}

boolean CSoundBaseDevice::AreChannelsSwapped (void) const
{
	return m_bSwapChannels;
}

unsigned CSoundBaseDevice::GetChunk (s16 *pBuffer, unsigned nChunkSize)
{
	assert (m_HWFormat == SoundFormatSigned16);

	return GetChunkInternal (pBuffer, nChunkSize);
}

unsigned CSoundBaseDevice::GetChunk (u32 *pBuffer, unsigned nChunkSize)
{
	assert (   m_HWFormat == SoundFormatSigned24
		|| m_HWFormat == SoundFormatUnsigned32
		|| m_HWFormat == SoundFormatIEC958);

	return GetChunkInternal (pBuffer, nChunkSize);
}

u32 CSoundBaseDevice::ConvertIEC958Sample (u32 nSample, unsigned nFrame)
{
	assert (m_HWFormat == SoundFormatIEC958);
	assert (nFrame < IEC958_FRAMES_PER_BLOCK);

	nSample &= 0xFFFFFF;
	nSample <<= 4;

	if (   nFrame < IEC958_STATUS_BYTES * 8
	    && (m_uchIEC958Status[nFrame / 8] & BIT(nFrame % 8)))
	{
		nSample |= 0x40000000;

	}

	if (parity32 (nSample))
	{
		nSample |= 0x80000000;
	}

	if (nFrame == 0)
	{
		nSample |= IEC958_B_FRAME_PREAMBLE;
	}

	return nSample;
}

void CSoundBaseDevice::ConvertSoundFormat (void *pTo, const void *pFrom)
{
	s32 nValue = 0;

	switch (m_WriteFormat)
	{
	case SoundFormatUnsigned8: {
		const u8 *pValue = reinterpret_cast<const u8 *> (pFrom);
		nValue = *pValue;
		nValue -= 128;
		nValue <<= 24;
		} break;

	case SoundFormatSigned16: {
		const s16 *pValue = reinterpret_cast<const s16 *> (pFrom);
		nValue = *pValue;
		nValue <<= 16;
		} break;

	case SoundFormatSigned24: {
		const u32 *pValue = reinterpret_cast<const u32 *> (pFrom);
		nValue = *pValue & 0xFFFFFF;
		nValue <<= 8;
		} break;

	case SoundFormatUnknown:
		memcpy (pTo, pFrom, m_nWriteSampleSize);
		return;

	default:
		assert (0);
		break;
	}

	switch (m_HWFormat)
	{
	case SoundFormatSigned16: {
		s16 *pValue = reinterpret_cast<s16 *> (pTo);
		*pValue = nValue >> 16;
		} break;

	case SoundFormatSigned24: {
		s32 *pValue = reinterpret_cast<s32 *> (pTo);
		*pValue = nValue >> 8;
		} break;

	case SoundFormatUnsigned32: {
		s64 llValue = (s64) nValue;
		llValue += 1U << 31;
		llValue *= m_nRangeMax;
		llValue >>= 32;

		u32 *pValue = reinterpret_cast<u32 *> (pTo);
		*pValue = (u32) llValue;
		} break;

	case SoundFormatIEC958: {
		nValue >>= 4;
		nValue &= 0xFFFFFF0;
		if (parity32 (nValue))
		{
			nValue |= 0x80000000;
		}

		s32 *pValue = reinterpret_cast<s32 *> (pTo);
		*pValue = nValue;
		} break;

	default:
		assert (0);
		break;
	}
}

unsigned CSoundBaseDevice::GetChunkInternal (void *pBuffer, unsigned nChunkSize)
{
	u8 *pBuffer8 = static_cast<u8 *> (pBuffer);
	assert (pBuffer8 != 0);

	assert (nChunkSize > 0);
	assert (nChunkSize % SOUND_HW_CHANNELS == 0);
	unsigned nChunkSizeBytes = nChunkSize * m_nHWSampleSize;

	m_SpinLock.Acquire ();

	unsigned nQueueBytesAvail = GetQueueBytesAvail ();
	unsigned nBytes = nQueueBytesAvail;
	if (nBytes > nChunkSizeBytes)
	{
		nBytes = nChunkSizeBytes;
	}

	if (nBytes > 0)
	{
		Dequeue (pBuffer8, nBytes);

		pBuffer8 += nBytes;
		nQueueBytesAvail -= nBytes;
	}

	m_SpinLock.Release ();

	while (nBytes < nChunkSizeBytes)
	{
		memcpy (pBuffer8, m_NullFrame, m_nHWFrameSize);

		pBuffer8 += m_nHWFrameSize;
		nBytes += m_nHWFrameSize;
	}

	// insert control channel and parity bits, and preamble into IEC958 block
	if (m_HWFormat == SoundFormatIEC958)
	{
		u32 *pBuffer32 = static_cast<u32 *> (pBuffer);

		unsigned i;
		for (i = 0; i < nChunkSize; i += IEC958_SUBFRAMES_PER_BLOCK)
		{
			unsigned j;
			for (j = 0; j < IEC958_STATUS_BYTES * 8 * SOUND_HW_CHANNELS; j++)
			{
				u32 *pSubFrame = &pBuffer32[i + j];

				unsigned nFrame = j / SOUND_HW_CHANNELS;
				if (m_uchIEC958Status[nFrame / 8] & BIT(nFrame % 8))
				{
					u32 nValue = *pSubFrame;

					nValue |= 0x40000000;

					nValue &= 0x7FFFFFFF;
					if (parity32 (nValue))
					{
						nValue |= 0x80000000;
					}

					*pSubFrame = nValue;
				}

				if (nFrame == 0)
				{
					*pSubFrame |= IEC958_B_FRAME_PREAMBLE;
				}
			}
		}

		assert (i == nChunkSize);	// nChunkSize must be a multiple of 384
	}

	if (   m_pCallback != 0
	    && nQueueBytesAvail < m_nNeedDataThreshold)
	{
		(*m_pCallback) (m_pCallbackParam);
	}

	return nChunkSize;
}

unsigned CSoundBaseDevice::GetQueueBytesFree (void)
{
	assert (m_nQueueSize > 1);
	assert (m_nInPtr < m_nQueueSize);
	assert (m_nOutPtr < m_nQueueSize);

	if (m_nOutPtr <= m_nInPtr)
	{
		return m_nQueueSize+m_nOutPtr-m_nInPtr-1;
	}

	return m_nOutPtr-m_nInPtr-1;
}

unsigned CSoundBaseDevice::GetQueueBytesAvail (void)
{
	assert (m_nQueueSize > 1);
	assert (m_nInPtr < m_nQueueSize);
	assert (m_nOutPtr < m_nQueueSize);

	if (m_nInPtr < m_nOutPtr)
	{
		return m_nQueueSize+m_nInPtr-m_nOutPtr;
	}

	return m_nInPtr-m_nOutPtr;
}

void CSoundBaseDevice::Enqueue (const void *pBuffer, unsigned nCount)
{
	const u8 *p = static_cast<const u8 *> (pBuffer);
	assert (p != 0);
	assert (m_pQueue != 0);

	assert (nCount > 0);
	while (nCount-- > 0)
	{
		m_pQueue[m_nInPtr] = *p++;

		if (++m_nInPtr == m_nQueueSize)
		{
			m_nInPtr = 0;
		}
	}
}

void CSoundBaseDevice::Dequeue (void *pBuffer, unsigned nCount)
{
	u8 *p = static_cast<u8 *> (pBuffer);
	assert (p != 0);
	assert (m_pQueue != 0);

	assert (nCount > 0);
	while (nCount-- > 0)
	{
		*p++ = m_pQueue[m_nOutPtr];

		if (++m_nOutPtr == m_nQueueSize)
		{
			m_nOutPtr = 0;
		}
	}
}
