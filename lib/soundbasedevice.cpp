//
// soundbasedevice.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017  R. Stange <rsta2@o2online.de>
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
#include <circle/util.h>
#include <assert.h>

#define THRESHOLD_MSECS		50	// sound data for this amount of time should be available

CSoundBaseDevice::CSoundBaseDevice (TSoundFormat HWFormat, u32 nRange32, unsigned nSampleRate)
:	m_HWFormat (HWFormat),
	m_nSampleRate (nSampleRate),
	m_nQueueSize (0),
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
	m_nNeedDataThreshold = m_nHWFrameSize * m_nSampleRate * THRESHOLD_MSECS/1000;

	m_nQueueSize = m_nNeedDataThreshold * 2;
	m_pQueue = new u8[m_nQueueSize];
	assert (m_pQueue != 0);

	SetWriteFormat (SoundFormatUnknown);
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

void CSoundBaseDevice::SetWriteFormat (TSoundFormat Format, unsigned nChannels)
{
	assert (Format <= SoundFormatUnknown);
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

	case SoundFormatUnknown:
		m_nWriteSampleSize = m_nHWSampleSize;
		break;

	default:
		assert (0);
		break;
	}

	m_nWriteFrameSize = m_nWriteChannels * m_nWriteSampleSize;
}

int CSoundBaseDevice::Write (const void *pBuffer, unsigned nCount)
{
	assert (pBuffer != 0);
	const u8 *pBuffer8 = static_cast<const u8 *> (pBuffer);

	int nResult = 0;

	m_SpinLock.Acquire ();

	while (   nCount >= m_nWriteFrameSize
	       && GetQueueBytesFree () >= m_nHWFrameSize)
	{
		u8 Frame[SOUND_MAX_FRAME_SIZE];

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

		Enqueue (Frame, m_nHWFrameSize);

		nCount -= m_nWriteFrameSize;
		nResult += m_nWriteFrameSize;
	}

	m_SpinLock.Release ();

	return nResult;
}

void CSoundBaseDevice::RegisterNeedDataCallback (TSoundNeedDataCallback *pCallback, void *pParam)
{
	assert (m_pCallback == 0);
	m_pCallback = pCallback;
	assert (m_pCallback != 0);

	m_pCallbackParam = pParam;
}

unsigned CSoundBaseDevice::GetChunk (s16 *pBuffer, unsigned nChunkSize)
{
	assert (m_HWFormat == SoundFormatSigned16);

	return GetChunkInternal (pBuffer, nChunkSize);
}

unsigned CSoundBaseDevice::GetChunk (u32 *pBuffer, unsigned nChunkSize)
{
	assert (   m_HWFormat == SoundFormatSigned24
		|| m_HWFormat == SoundFormatUnsigned32);

	return GetChunkInternal (pBuffer, nChunkSize);
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

	default:
		assert (0);
		break;
	}
}

unsigned CSoundBaseDevice::GetChunkInternal (void *pBuffer, unsigned nChunkSize)
{
	u8 *pBuffer8 = static_cast<u8 *> (pBuffer);
	assert (pBuffer8 != 0);

	unsigned nResult = 0;

	m_SpinLock.Acquire ();

	assert (nChunkSize > 0);
	while (  nChunkSize >= SOUND_HW_CHANNELS
	       && GetQueueBytesAvail () >= m_nHWFrameSize)
	{
		Dequeue (pBuffer8, m_nHWFrameSize);

		pBuffer8 += m_nHWFrameSize;
		nChunkSize -= SOUND_HW_CHANNELS;
		nResult += SOUND_HW_CHANNELS;
	}

	m_SpinLock.Release ();

	while (nChunkSize >= SOUND_HW_CHANNELS)
	{
		memcpy (pBuffer8, m_NullFrame, m_nHWFrameSize);

		pBuffer8 += m_nHWFrameSize;
		nChunkSize -= SOUND_HW_CHANNELS;
		nResult += SOUND_HW_CHANNELS;
	}

	if (   m_pCallback != 0
	    && GetQueueBytesAvail () < m_nNeedDataThreshold)
	{
		(*m_pCallback) (m_pCallbackParam);
	}

	return nResult;
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
