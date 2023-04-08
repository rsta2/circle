//
// soundbasedevice.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2023  R. Stange <rsta2@o2online.de>
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
#include <circle/sound/soundbasedevice.h>
#include <circle/macros.h>
#include <circle/util.h>
#include <assert.h>

CSoundBaseDevice::CSoundBaseDevice (void)
:	m_HWFormat (SoundFormatUnknown),
	m_nQueueSize (0),
	m_nNeedDataThreshold (0),
	m_WriteFormat (SoundFormatUnknown),
	m_nWriteChannels (0),
	m_pQueue (0),
	m_nInPtr (0),
	m_nOutPtr (0),
	m_pCallback (0),
	m_nReadQueueSize (0),
	m_nHaveDataThreshold (0),
	m_ReadFormat (SoundFormatUnknown),
	m_nReadChannels (0),
	m_pReadQueue (0),
	m_nReadInPtr (0),
	m_nReadOutPtr (0),
	m_pReadCallback (0),
	m_pReadCallbackParam (0)
{
}

CSoundBaseDevice::CSoundBaseDevice (TSoundFormat HWFormat, u32 nRange32, unsigned nSampleRate,
				    boolean bSwapChannels)
:	m_HWFormat (SoundFormatUnknown),
	m_nQueueSize (0),
	m_nNeedDataThreshold (0),
	m_WriteFormat (SoundFormatUnknown),
	m_nWriteChannels (0),
	m_pQueue (0),
	m_nInPtr (0),
	m_nOutPtr (0),
	m_pCallback (0),
	m_nReadQueueSize (0),
	m_nHaveDataThreshold (0),
	m_ReadFormat (SoundFormatUnknown),
	m_nReadChannels (0),
	m_pReadQueue (0),
	m_nReadInPtr (0),
	m_nReadOutPtr (0),
	m_pReadCallback (0),
	m_pReadCallbackParam (0)
{
	Setup (HWFormat, nRange32, nSampleRate, 2, 2, bSwapChannels);
}

CSoundBaseDevice::~CSoundBaseDevice (void)
{
	m_pCallback = 0;
	m_pReadCallback = 0;

	delete [] m_pQueue;
	m_pQueue = 0;
	delete [] m_pReadQueue;
	m_pReadQueue = 0;
}

void CSoundBaseDevice::Setup (TSoundFormat HWFormat, u32 nRange32, unsigned nSampleRate,
			      unsigned nHWTXChannels, unsigned nHWRXChannels,
			      boolean bSwapChannels)
{
	m_HWFormat = HWFormat;
	m_nSampleRate = nSampleRate;

	m_nHWTXChannels = nHWTXChannels;
	m_nHWRXChannels = nHWRXChannels;
	assert (m_nHWTXChannels <= SOUND_MAX_CHANNELS);
	assert (m_nHWRXChannels <= SOUND_MAX_CHANNELS);

	m_bSwapChannels = bSwapChannels;
	assert (!m_bSwapChannels || m_nHWTXChannels == 2);

	memset (m_NullFrame, 0, sizeof m_NullFrame);

	switch (m_HWFormat)
	{
	case SoundFormatSigned16:
		m_nHWSampleSize = sizeof (s16);
		m_nRangeMin = -32768;
		m_nRangeMax = 32767;
		break;

	case SoundFormatSigned24:
		m_nHWSampleSize = sizeof (u8)*3;
		m_nRangeMin = -(1 << 23)+1;
		m_nRangeMax = (1 << 23)-1;
		break;

	case SoundFormatSigned24_32:
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
		for (unsigned i = 0; i < m_nHWTXChannels; i++)
		{
			pSample32[i] = m_nRangeMax / 2;
		}
		} break;

	default:
		assert (0);
		break;
	}

	m_nHWTXFrameSize = m_nHWTXChannels * m_nHWSampleSize;
	m_nHWRXFrameSize = m_nHWRXChannels * m_nHWSampleSize;

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

unsigned CSoundBaseDevice::GetHWTXChannels (void) const
{
	return m_nHWTXChannels;
}

unsigned CSoundBaseDevice::GetHWRXChannels (void) const
{
	return m_nHWRXChannels;
}

int CSoundBaseDevice::GetRangeMin (void) const
{
	return m_nRangeMin;
}

int CSoundBaseDevice::GetRangeMax (void) const
{
	return m_nRangeMax;
}

// Output /////////////////////////////////////////////////////////////

boolean CSoundBaseDevice::AllocateQueue (unsigned nSizeMsecs)
{
	assert (m_pQueue == 0);
	assert (1 <= nSizeMsecs && nSizeMsecs <= 1000);

	// 1 byte remains free
	m_nQueueSize = (m_nHWTXFrameSize*m_nSampleRate*nSizeMsecs + 999) / 1000 + 1;

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
	m_nQueueSize = m_nHWTXFrameSize*nSizeFrames + 1;

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

	assert (1 <= nChannels && nChannels <= SOUND_MAX_CHANNELS);
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

	case SoundFormatSigned24_32:
		m_nWriteSampleSize = sizeof (s32);
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
	    && m_nWriteChannels == m_nHWTXChannels
	    && !m_bSwapChannels)
	{
		// fast path without bit depth conversion or channel swapping

		unsigned nBytes = GetQueueBytesFree ();
		if (nBytes > nCount)
		{
			nBytes = nCount;
		}
		nBytes -= nBytes % m_nWriteFrameSize;	// must be a multiple of frame size

		if (nBytes > 0)
		{
			Enqueue (pBuffer, nBytes);

			nResult = nBytes;
		}
	}
	else if (   m_nHWTXChannels == 2
		 && m_nWriteChannels <= 2)
	{
		while (   nCount >= m_nWriteFrameSize
		       && GetQueueBytesFree () >= m_nHWTXFrameSize)
		{
			u8 Frame[2 * SOUND_MAX_SAMPLE_SIZE];

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

			Enqueue (Frame, m_nHWTXFrameSize);

			nCount -= m_nWriteFrameSize;
			nResult += m_nWriteFrameSize;
		}
	}
	else
	{
		unsigned nMinChannels = m_nHWTXChannels;
		unsigned nNullBytes = 0;
		if (m_nWriteChannels < m_nHWTXChannels)
		{
			nMinChannels = m_nWriteChannels;
			nNullBytes = (m_nHWTXChannels - m_nWriteChannels) * m_nHWSampleSize;
		}

		while (   nCount >= m_nWriteFrameSize
		       && GetQueueBytesFree () >= m_nHWTXFrameSize)
		{
			u8 Frame[SOUND_MAX_FRAME_SIZE];
			u8 *pFrame = Frame;

			for (unsigned i = 0; i < nMinChannels; i++)
			{
				ConvertSoundFormat (pFrame, pBuffer8);

				pFrame += m_nHWSampleSize;
				pBuffer8 += m_nWriteSampleSize;
			}

			if (nNullBytes != 0)
			{
				memcpy (pFrame, m_NullFrame, nNullBytes);
			}

			Enqueue (Frame, m_nHWTXFrameSize);

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
	return m_nQueueSize / m_nHWTXFrameSize;
}

unsigned CSoundBaseDevice::GetQueueFramesAvail (void)
{
	assert (m_nQueueSize > 0);

	m_SpinLock.Acquire ();

	unsigned nQueueBytesAvail = GetQueueBytesAvail ();

	m_SpinLock.Release ();

	return nQueueBytesAvail / m_nHWTXFrameSize;
}

void CSoundBaseDevice::RegisterNeedDataCallback (TSoundDataCallback *pCallback, void *pParam)
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

// Input //////////////////////////////////////////////////////////////

boolean CSoundBaseDevice::AllocateReadQueue (unsigned nSizeMsecs)
{
	assert (m_pReadQueue == 0);
	assert (1 <= nSizeMsecs && nSizeMsecs <= 1000);

	// 1 byte remains free
	m_nReadQueueSize = (m_nHWRXFrameSize*m_nSampleRate*nSizeMsecs + 999) / 1000 + 1;

	m_pReadQueue = new u8[m_nReadQueueSize];
	if (m_pReadQueue == 0)
	{
		return FALSE;
	}

	m_nHaveDataThreshold = m_nReadQueueSize / 2;

	return TRUE;
}

boolean CSoundBaseDevice::AllocateReadQueueFrames (unsigned nSizeFrames)
{
	assert (m_pReadQueue == 0);
	assert (1 <= nSizeFrames && nSizeFrames <= m_nSampleRate);

	// 1 byte remains free
	m_nReadQueueSize = m_nHWRXFrameSize*nSizeFrames + 1;

	m_pReadQueue = new u8[m_nReadQueueSize];
	if (m_pReadQueue == 0)
	{
		return FALSE;
	}

	m_nHaveDataThreshold = m_nReadQueueSize / 2;

	return TRUE;
}

void CSoundBaseDevice::SetReadFormat (TSoundFormat Format, unsigned nChannels,
				      boolean bLeftChannel)
{
	assert (Format < SoundFormatUnknown);
	m_ReadFormat = Format;

	assert (1 <= nChannels && nChannels <= SOUND_MAX_CHANNELS);
	m_nReadChannels = nChannels;
	m_bLeftChannel = bLeftChannel;

	switch (m_ReadFormat)
	{
	case SoundFormatUnsigned8:
		m_nReadSampleSize = sizeof (u8);
		break;

	case SoundFormatSigned16:
		m_nReadSampleSize = sizeof (s16);
		break;

	case SoundFormatSigned24:
		m_nReadSampleSize = sizeof (u8)*3;
		break;

	case SoundFormatSigned24_32:
		m_nReadSampleSize = sizeof (s32);
		break;

	default:
		assert (0);
		break;
	}

	m_nReadFrameSize = m_nReadChannels * m_nReadSampleSize;
}

int CSoundBaseDevice::Read (void *pBuffer, size_t nCount)
{
	assert (m_ReadFormat < SoundFormatUnknown);

	assert (pBuffer != 0);
	u8 *pBuffer8 = static_cast<u8 *> (pBuffer);

	int nResult = 0;

	m_ReadSpinLock.Acquire ();

	if (   m_HWFormat == m_ReadFormat
	    && m_nReadChannels == m_nHWRXChannels)
	{
		// fast path without bit depth conversion

		unsigned nBytes = GetReadQueueBytesAvail ();
		if (nBytes > nCount)
		{
			nBytes = nCount;
		}
		nBytes -= nBytes % m_nReadFrameSize;	// must be a multiple of frame size

		if (nBytes > 0)
		{
			ReadDequeue (pBuffer, nBytes);

			nResult = nBytes;
		}
	}
	else if (   m_nHWRXChannels == 2
		 && m_nReadChannels <= 2)
	{
		while (   nCount >= m_nReadFrameSize
		       && GetReadQueueBytesAvail () >= m_nHWRXFrameSize)
		{
			u8 Frame[SOUND_MAX_FRAME_SIZE];

			ReadDequeue (Frame, m_nHWRXFrameSize);

			if (m_nReadChannels == 2)
			{
				ConvertReadSoundFormat (pBuffer8, Frame);
				pBuffer8 += m_nReadSampleSize;

				ConvertReadSoundFormat (pBuffer8, Frame+m_nHWSampleSize);
				pBuffer8 += m_nReadSampleSize;
			}
			else
			{
				if (m_bLeftChannel)
				{
					ConvertReadSoundFormat (pBuffer8, Frame);
				}
				else
				{
					ConvertReadSoundFormat (pBuffer8, Frame+m_nHWSampleSize);
				}

				pBuffer8 += m_nReadSampleSize;
			}

			nCount -= m_nReadFrameSize;
			nResult += m_nReadFrameSize;
		}
	}
	else
	{
		u8 NullSample[SOUND_MAX_SAMPLE_SIZE];

		unsigned nMinChannels = m_nHWRXChannels;
		if (m_nReadChannels < m_nHWRXChannels)
		{
			nMinChannels = m_nReadChannels;
		}
		else
		{
			ConvertReadSoundFormat (NullSample, m_NullFrame);
		}

		while (   nCount >= m_nReadFrameSize
		       && GetReadQueueBytesAvail () >= m_nHWRXFrameSize)
		{
			u8 Frame[SOUND_MAX_FRAME_SIZE];
			u8 *pFrame = Frame;

			ReadDequeue (Frame, m_nHWRXFrameSize);

			unsigned i = 0;
			for (; i < nMinChannels; i++)
			{
				ConvertReadSoundFormat (pBuffer8, pFrame);

				pBuffer8 += m_nReadSampleSize;
				pFrame += m_nHWSampleSize;
			}

			for (; i < m_nReadChannels; i++)
			{
				memcpy (pBuffer8, NullSample, m_nReadSampleSize);

				pBuffer8 += m_nReadSampleSize;
			}

			nCount -= m_nReadFrameSize;
			nResult += m_nReadFrameSize;
		}
	}

	m_ReadSpinLock.Release ();

	return nResult;
}

unsigned CSoundBaseDevice::GetReadQueueSizeFrames (void)
{
	assert (m_nReadQueueSize > 0);
	return m_nReadQueueSize / m_nHWRXFrameSize;
}

unsigned CSoundBaseDevice::GetReadQueueFramesAvail (void)
{
	assert (m_nReadQueueSize > 0);

	m_ReadSpinLock.Acquire ();

	unsigned nReadQueueBytesAvail = GetReadQueueBytesAvail ();

	m_ReadSpinLock.Release ();

	return nReadQueueBytesAvail / m_nHWRXFrameSize;
}

void CSoundBaseDevice::RegisterHaveDataCallback (TSoundDataCallback *pCallback, void *pParam)
{
	assert (m_pReadCallback == 0);
	m_pReadCallback = pCallback;
	assert (m_pReadCallback != 0);

	m_pReadCallbackParam = pParam;
}

// Output /////////////////////////////////////////////////////////////

unsigned CSoundBaseDevice::GetChunk (s16 *pBuffer, unsigned nChunkSize)
{
	assert (m_HWFormat == SoundFormatSigned16);

	return GetChunkInternal (pBuffer, nChunkSize);
}

unsigned CSoundBaseDevice::GetChunk (u32 *pBuffer, unsigned nChunkSize)
{
	assert (   m_HWFormat == SoundFormatSigned24
		|| m_HWFormat == SoundFormatSigned24_32
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

	case SoundFormatSigned24:
	case SoundFormatSigned24_32: {
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

	case SoundFormatSigned24:
	case SoundFormatSigned24_32: {
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
	assert (nChunkSize % m_nHWTXChannels == 0);
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
		memcpy (pBuffer8, m_NullFrame, m_nHWTXFrameSize);

		pBuffer8 += m_nHWTXFrameSize;
		nBytes += m_nHWTXFrameSize;
	}

	// insert control channel and parity bits, and preamble into IEC958 block
	if (m_HWFormat == SoundFormatIEC958)
	{
		u32 *pBuffer32 = static_cast<u32 *> (pBuffer);

		unsigned i;
		for (i = 0; i < nChunkSize; i += IEC958_SUBFRAMES_PER_BLOCK)
		{
			unsigned j;
			for (j = 0; j < IEC958_STATUS_BYTES * 8 * m_nHWTXChannels; j++)
			{
				u32 *pSubFrame = &pBuffer32[i + j];

				unsigned nFrame = j / m_nHWTXChannels;
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

// Input //////////////////////////////////////////////////////////////

void CSoundBaseDevice::PutChunk (const s16 *pBuffer, unsigned nChunkSize)
{
	assert (m_HWFormat == SoundFormatSigned16);

	PutChunkInternal (pBuffer, nChunkSize);
}

void CSoundBaseDevice::PutChunk (const u32 *pBuffer, unsigned nChunkSize)
{
	assert (   m_HWFormat == SoundFormatSigned24
		|| m_HWFormat == SoundFormatSigned24_32);

	PutChunkInternal (pBuffer, nChunkSize);
}

void CSoundBaseDevice::PutChunkInternal (const void *pBuffer, unsigned nChunkSize)
{
	const u8 *pBuffer8 = reinterpret_cast<const u8 *> (pBuffer);
	assert (pBuffer8 != 0);

	assert (nChunkSize > 0);
	assert (nChunkSize % m_nHWRXChannels == 0);
	unsigned nChunkSizeBytes = nChunkSize * m_nHWSampleSize;

	m_ReadSpinLock.Acquire ();

	unsigned nReadQueueBytesFree = GetReadQueueBytesFree ();
	unsigned nBytes = nReadQueueBytesFree;
	if (nBytes > nChunkSizeBytes)
	{
		nBytes = nChunkSizeBytes;
	}

	if (nBytes > 0)
	{
		ReadEnqueue (pBuffer8, nBytes);

		nReadQueueBytesFree -= nBytes;
	}

	m_ReadSpinLock.Release ();

	if (   m_pReadCallback != 0
	    && nReadQueueBytesFree < m_nHaveDataThreshold)
	{
		(*m_pReadCallback) (m_pReadCallbackParam);
	}
}

void CSoundBaseDevice::ConvertReadSoundFormat (void *pTo, const void *pFrom)
{
	u32 nValue = 0;

	switch (m_HWFormat)
	{
	case SoundFormatSigned16: {
		const s16 *pValue = reinterpret_cast<const s16 *> (pFrom);
		nValue = *pValue;
		nValue <<= 8;
		} break;

	case SoundFormatSigned24: {
		const u32 *pValue = reinterpret_cast<const u32 *> (pFrom);
		nValue = *pValue & 0xFFFFFFU;
		} break;

	case SoundFormatSigned24_32: {
		const u32 *pValue = reinterpret_cast<const u32 *> (pFrom);
		nValue = *pValue;
		} break;

	default:
		assert (0);
		break;
	}

	switch (m_ReadFormat)
	{
	case SoundFormatUnsigned8: {
		u8 *pValue = reinterpret_cast<u8 *> (pTo);
		s8 chValue = (s8) (nValue >> 16);
		*pValue = (u8) (128 + chValue);
		} break;

	case SoundFormatSigned16: {
		s16 *pValue = reinterpret_cast<s16 *> (pTo);
		*pValue = (s16) (nValue >> 8);
		} break;

	case SoundFormatSigned24:
	case SoundFormatSigned24_32: {
		u32 *pValue = reinterpret_cast<u32 *> (pTo);
		*pValue = nValue;
		} break;

	default:
		assert (0);
		break;
	}
}

unsigned CSoundBaseDevice::GetReadQueueBytesFree (void)
{
	assert (m_nReadQueueSize > 1);
	assert (m_nReadInPtr < m_nReadQueueSize);
	assert (m_nReadOutPtr < m_nReadQueueSize);

	if (m_nReadOutPtr <= m_nReadInPtr)
	{
		return m_nReadQueueSize+m_nReadOutPtr-m_nReadInPtr-1;
	}

	return m_nReadOutPtr-m_nReadInPtr-1;
}

unsigned CSoundBaseDevice::GetReadQueueBytesAvail (void)
{
	assert (m_nReadQueueSize > 1);
	assert (m_nReadInPtr < m_nReadQueueSize);
	assert (m_nReadOutPtr < m_nReadQueueSize);

	if (m_nReadInPtr < m_nReadOutPtr)
	{
		return m_nReadQueueSize+m_nReadInPtr-m_nReadOutPtr;
	}

	return m_nReadInPtr-m_nReadOutPtr;
}

void CSoundBaseDevice::ReadEnqueue (const void *pBuffer, unsigned nCount)
{
	const u8 *p = static_cast<const u8 *> (pBuffer);
	assert (p != 0);
	assert (m_pReadQueue != 0);

	assert (nCount > 0);
	while (nCount-- > 0)
	{
		m_pReadQueue[m_nReadInPtr] = *p++;

		if (++m_nReadInPtr == m_nReadQueueSize)
		{
			m_nReadInPtr = 0;
		}
	}
}

void CSoundBaseDevice::ReadDequeue (void *pBuffer, unsigned nCount)
{
	u8 *p = static_cast<u8 *> (pBuffer);
	assert (p != 0);
	assert (m_pReadQueue != 0);

	assert (nCount > 0);
	while (nCount-- > 0)
	{
		*p++ = m_pReadQueue[m_nReadOutPtr];

		if (++m_nReadOutPtr == m_nReadQueueSize)
		{
			m_nReadOutPtr = 0;
		}
	}
}
