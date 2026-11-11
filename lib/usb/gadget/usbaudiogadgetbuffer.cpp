// SPDX-License-Identifier: GPL-3.0-or-later
//
// usbaudiogadgetbuffer.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
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
#include <circle/usb/gadget/usbaudiogadgetbuffer.h>
#include <circle/util.h>

using TFunction = CUSBAudioGadgetFunction;

CUSBAudioGadgetBuffer::CUSBAudioGadgetBuffer (unsigned nOutputRate, unsigned nChannels,
					  unsigned nBufferFrames)
:	m_nOutputRate (nOutputRate), m_nChannels (nChannels), m_nCapacity (nBufferFrames),
	m_pBuffer (nullptr), m_nHead (0), m_nTail (0), m_nCount (0),
	m_bConfigured (FALSE), m_bStarted (FALSE), m_Stream {},
	m_nPhase (0), m_nStep (0), m_nCorrectionPPM (0), m_nGain (65536)
{
	if (nOutputRate >= 8000 && nOutputRate <= 192000
	    && nChannels && nChannels <= TFunction::MaxChannels
	    && nBufferFrames >= 16 && nBufferFrames <= 16384)
	{
		m_pBuffer = new s32[nBufferFrames*nChannels];
	}
}

CUSBAudioGadgetBuffer::~CUSBAudioGadgetBuffer (void)
{
	delete [] m_pBuffer;
}

boolean CUSBAudioGadgetBuffer::IsValid (void) const
{
	return m_pBuffer != nullptr;
}

void CUSBAudioGadgetBuffer::Stop (void)
{
	m_Lock.Acquire ();
	m_bConfigured = m_bStarted = FALSE;
	m_nHead = m_nTail = m_nCount = 0;
	m_nPhase = 0;
	m_nCorrectionPPM = 0;
	m_Lock.Release ();
}

boolean CUSBAudioGadgetBuffer::Configure (const TFunction::TStream &Stream)
{
	Stop ();
	if (!m_pBuffer || Stream.nChannels != m_nChannels
	    || (Stream.nSampleRate + 999ULL) / 1000 > m_nCapacity
	    || !TFunction::GetPacketSize (Stream.nSampleRate, Stream.nChannels, Stream.Format))
	{
		return FALSE;
	}
	m_Lock.Acquire ();
	m_Stream = Stream;
	m_nStep = ((u64) Stream.nSampleRate << 32) / m_nOutputRate;
	m_bConfigured = TRUE;
	m_Lock.Release ();
	return TRUE;
}

void CUSBAudioGadgetBuffer::SetControls (boolean bMute, int nVolume)
{
	if (nVolume > 0)
	{
		nVolume = 0;
	}
	if (nVolume < -96*256)
	{
		nVolume = -96*256;
	}
	unsigned nGain = 65536;
	// -1 dB step. Control changes happen outside the real-time audio callback.
	for (int n = 0; n > nVolume; n -= 256)
	{
		nGain = (unsigned) ((u64) nGain * 58409 / 65536);
	}
	m_Lock.Acquire ();
	m_nGain = bMute ? 0 : nGain;
	m_Lock.Release ();
}

s32 CUSBAudioGadgetBuffer::DecodeSample (const u8 *p, TFunction::TFormat Format)
{
	if (!p || !TFunction::GetSubframeSize (Format))
	{
		return 0;
	}
	if (Format == TFunction::PCM8)
	{
		return (p[0] >= 128 ? (s32) p[0]-256 : (s32) p[0]) * 16777216;
	}
	if (Format == TFunction::PCM16)
	{
		unsigned n = p[0] | (unsigned) p[1]<<8;
		return (n >= 32768 ? (s32) n-65536 : (s32) n) * 65536;
	}
	if (Format == TFunction::PCM24)
	{
		unsigned n = p[0] | (unsigned) p[1]<<8 | (unsigned) p[2]<<16;
		return (n >= 0x800000 ? (s32) n-0x1000000 : (s32) n) * 256;
	}
	u32 n = p[0] | (u32) p[1]<<8 | (u32) p[2]<<16 | (u32) p[3]<<24;
	if (Format == TFunction::Float32)
	{
		unsigned nExponent = (n>>23) & 255;
		boolean bNegative = n>>31;
		if (nExponent == 255 || nExponent < 96)
		{
			return 0;
		}
		if (nExponent >= 127)
		{
			return bNegative ? (-2147483647-1) : 2147483647;
		}
		s32 nMagnitude = (s32) (((n & 0x7FFFFF) | 0x800000) << 8 >> (127-nExponent));
		return bNegative ? -nMagnitude : nMagnitude;
	}
	if (Format != TFunction::PCM32)
	{
		return 0;
	}
	return n & 0x80000000 ? -(s32) ~n - 1 : (s32) n;
}

boolean CUSBAudioGadgetBuffer::Write (const void *pData, unsigned nFrames,
				    const TFunction::TStream &Stream)
{
	m_Lock.Acquire ();
	if (!m_bConfigured || !pData || Stream.Format != m_Stream.Format
	    || Stream.nChannels != m_nChannels || Stream.nSampleRate != m_Stream.nSampleRate
	    || nFrames > m_nCapacity)
	{
		m_Lock.Release ();
		return FALSE;
	}
	boolean bOK = nFrames <= m_nCapacity-m_nCount;
	if (!bOK)
	{
		// On a scheduling stall discard backlog, then prefill again.
		m_nHead = m_nTail = m_nCount = 0;
		m_nPhase = 0;
		m_bStarted = FALSE;
	}
	const u8 *p = (const u8 *) pData;
	if (Stream.Format == TFunction::PCM32)
	{
		// Circle runs little-endian: signed PCM32 already has our internal
		// representation. memcpy also accepts unaligned USB packet data.
		// At most two spans are needed, since nFrames <= m_nCapacity.
		unsigned nFirst = m_nCapacity-m_nHead;
		if (nFirst > nFrames)
		{
			nFirst = nFrames;
		}
		unsigned nFrameBytes = m_nChannels * sizeof (s32);
		if (nFirst)
		{
			memcpy (m_pBuffer+m_nHead*m_nChannels, p, nFirst*nFrameBytes);
		}
		unsigned nRemaining = nFrames-nFirst;
		if (nRemaining)
		{
			memcpy (m_pBuffer, p+nFirst*nFrameBytes, nRemaining*nFrameBytes);
		}
		m_nHead += nFrames;
		if (m_nHead >= m_nCapacity)
		{
			m_nHead -= m_nCapacity;
		}
	}
	else
	{
		unsigned nBytes = TFunction::GetSubframeSize (Stream.Format);
		for (unsigned i = 0; i < nFrames; i++)
		{
			for (unsigned j = 0; j < m_nChannels; j++, p += nBytes)
			{
				m_pBuffer[m_nHead*m_nChannels+j] = DecodeSample (p, Stream.Format);
			}
			if (++m_nHead == m_nCapacity)
			{
				m_nHead = 0;
			}
		}
	}
	m_nCount += nFrames;
	m_Lock.Release ();
	return bOK;
}

unsigned CUSBAudioGadgetBuffer::ReadFrames (s32 *pBuffer, unsigned nFrames)
{
	if (!pBuffer || !m_nChannels || m_nChannels > TFunction::MaxChannels)
	{
		return 0;
	}
	m_Lock.Acquire ();
	unsigned nRendered = 0;
	if (m_bConfigured && !m_bStarted && m_nCount >= m_nCapacity/2)
	{
		m_bStarted = TRUE;
	}
	if (m_bStarted)
	{
		// Correct small clock mismatch without discrete frame slips. Bounded to
		// +/-2000 ppm; slew at 100 ppm per callback. Nominal-rate conversion is
		// separate from this queue-fill correction.
		int nError = (int) m_nCount - (int) (m_nCapacity/2);
		int nTarget = nError*4000/(int) m_nCapacity;
		if (nTarget > 2000)
		{
			nTarget = 2000;
		}
		if (nTarget < -2000)
		{
			nTarget = -2000;
		}
		if (nTarget > m_nCorrectionPPM+100)
		{
			nTarget = m_nCorrectionPPM+100;
		}
		if (nTarget < m_nCorrectionPPM-100)
		{
			nTarget = m_nCorrectionPPM-100;
		}
		m_nCorrectionPPM = nTarget;
	}
	u64 nStep = m_nStep * (unsigned) (1000000+m_nCorrectionPPM) / 1000000;
	for (unsigned i = 0; i < nFrames; i++)
	{
		if (m_bConfigured && m_bStarted && m_nCount >= 2)
		{
			unsigned nNext = m_nTail+1;
			if (nNext == m_nCapacity)
			{
				nNext = 0;
			}
			unsigned nFraction = (unsigned) m_nPhase >> 16;
			for (unsigned j = 0; j < m_nChannels; j++)
			{
				s64 a = m_pBuffer[m_nTail*m_nChannels+j];
				s64 b = m_pBuffer[nNext*m_nChannels+j];
				s64 nSample = a + (b-a)*nFraction/65536;
				*pBuffer++ = (s32) (nSample*m_nGain/65536);
			}
			++nRendered;
			m_nPhase += nStep;
			unsigned nAdvance = (unsigned) (m_nPhase >> 32);
			m_nPhase &= 0xFFFFFFFF;
			if (nAdvance >= m_nCount)
			{
				m_nTail = m_nHead;
				m_nCount = 0;
				m_bStarted = FALSE;
				m_nPhase = 0;
			}
			else
			{
				// Here nAdvance < m_nCount <= m_nCapacity, so one
				// subtraction handles both single- and multi-frame advances.
				m_nTail += nAdvance;
				if (m_nTail >= m_nCapacity)
				{
					m_nTail -= m_nCapacity;
				}
				m_nCount -= nAdvance;
			}
		}
		else
		{
			m_bStarted = FALSE;
			for (unsigned j = 0; j < m_nChannels; j++)
			{
				*pBuffer++ = 0;
			}
		}
	}
	m_Lock.Release ();
	return nRendered;
}

unsigned CUSBAudioGadgetBuffer::GetBufferedFrames (void)
{
	m_Lock.Acquire ();
	unsigned n = m_nCount;
	m_Lock.Release ();
	return n;
}

int CUSBAudioGadgetBuffer::GetCorrectionPPM (void)
{
	m_Lock.Acquire ();
	int n = m_nCorrectionPPM;
	m_Lock.Release ();
	return n;
}
