// SPDX-License-Identifier: GPL-3.0-or-later
//
// usbaudiogadgetfunction.cpp
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
#include <circle/usb/gadget/usbaudiogadgetfunction.h>
#include <circle/usb/usbaudio.h>
#include <circle/util.h>

namespace
{
const int VolumeMinimum = -96 * 256;
const int VolumeResolution = 256;

// The descriptors are assembled bytewise: no packed/unaligned host accesses.
class CDescriptorWriter
{
public:
	CDescriptorWriter (u8 *pBuffer)
	:	m_pBuffer (pBuffer),
		m_nLength (0)
	{
	}

	void Byte (unsigned n)
	{
		m_pBuffer[m_nLength++] = (u8) n;
	}

	void Word (unsigned n)
	{
		Byte (n);
		Byte (n >> 8);
	}

	void Triple (unsigned n)
	{
		Word (n);
		Byte (n >> 16);
	}

	unsigned Length (void) const
	{
		return m_nLength;
	}

private:
	u8 *m_pBuffer;
	unsigned m_nLength;
};
}

CUSBAudioGadgetFunction::CUSBAudioGadgetFunction (const TConfiguration &Config, CSink *pSink)
:	m_Config (Config), m_pSink (pSink), m_bValid (FALSE),
	m_bConfigured (FALSE), m_bSuspended (FALSE), m_bReady (FALSE),
	m_nAlternate (0), m_nRate (0), m_nRevision (1), m_nAppliedRevision (0),
	m_bMute (FALSE), m_nVolume (0), m_bControlsPending (TRUE),
	m_Stream {PCM16, 0, 0}, m_nHead (0), m_nTail (0), m_nCount (0),
	m_Statistics {}
{
	if (!pSink || Config.nControlInterface > 255 || Config.nStreamingInterface > 255
	    || Config.nControlInterface == Config.nStreamingInterface
	    || !Config.nEndpoint || Config.nEndpoint > 7
	    || !Config.nInputTerminal || Config.nInputTerminal > USB_AUDIO_MAXIMUM_UNIT_ID
	    || !Config.nFeatureUnit || Config.nFeatureUnit > USB_AUDIO_MAXIMUM_UNIT_ID
	    || !Config.nOutputTerminal || Config.nOutputTerminal > USB_AUDIO_MAXIMUM_UNIT_ID
	    || Config.nInputTerminal == Config.nFeatureUnit
	    || Config.nInputTerminal == Config.nOutputTerminal
	    || Config.nFeatureUnit == Config.nOutputTerminal
	    || !Config.nChannels || Config.nChannels > MaxChannels
	    || !Config.nAlternates || Config.nAlternates > MaxAlternates
	    || (Config.wChannelConfig & ~0xFFF))
	{
		return;
	}
	unsigned nMapped = 0;
	for (unsigned n = Config.wChannelConfig; n; n >>= 1)
	{
		nMapped += n & 1;
	}
	if (nMapped > Config.nChannels)
	{
		return;
	}
	for (unsigned i = 0; i < Config.nAlternates; i++)
	{
		const TAlternate &Alt = Config.Alternates[i];
		if (!GetSubframeSize (Alt.Format) || !Alt.nRates || Alt.nRates > MaxRates)
		{
			return;
		}
		for (unsigned j = 0; j < i; j++)
		{
			if (Config.Alternates[j].Format == Alt.Format)
			{
				return;
			}
		}
		for (unsigned j = 0; j < Alt.nRates; j++)
		{
			if (!GetPacketSize (Alt.Rates[j], Config.nChannels, Alt.Format)
			    || (j && Alt.Rates[j] <= Alt.Rates[j-1]))
			{
				return;
			}
		}
	}
	m_nRate = Config.Alternates[0].Rates[0];
	m_bValid = TRUE;
}

boolean CUSBAudioGadgetFunction::IsValid (void) const
{
	return m_bValid;
}

unsigned CUSBAudioGadgetFunction::GetSubframeSize (TFormat Format)
{
	switch (Format)
	{
	case PCM8:
	case U8: return 1;
	case PCM16: return 2;
	case PCM24: return 3;
	case PCM32:
	case Float32: return 4;
	default: return 0;
	}
}

unsigned CUSBAudioGadgetFunction::GetPacketSize (unsigned nRate, unsigned nChannels,
						 TFormat Format)
{
	unsigned nBytes = GetSubframeSize (Format);
	if (!nRate || nRate > 0x7FFFFF || !nChannels || nChannels > MaxChannels || !nBytes)
	{
		return 0;
	}
	unsigned nPacket = ((nRate + 999) / 1000) * nChannels * nBytes;
	return nPacket <= MaxPacketSize ? nPacket : 0;
}

unsigned CUSBAudioGadgetFunction::PacketSize (unsigned nAlternate) const
{
	const TAlternate &Alt = m_Config.Alternates[nAlternate];
	return GetPacketSize (Alt.Rates[Alt.nRates-1], m_Config.nChannels, Alt.Format);
}

unsigned CUSBAudioGadgetFunction::GetAlternatePacketSize (unsigned nAlternate) const
{
	return m_bValid && nAlternate && nAlternate <= m_Config.nAlternates
		? PacketSize (nAlternate-1) : 0;
}

unsigned CUSBAudioGadgetFunction::GetMaxPacketSize (void) const
{
	unsigned nMaximum = 0;
	if (m_bValid)
	{
		for (unsigned i = 0; i < m_Config.nAlternates; i++)
		{
			unsigned nSize = PacketSize (i);
			if (nSize > nMaximum)
			{
				nMaximum = nSize;
			}
		}
	}
	return nMaximum;
}

unsigned CUSBAudioGadgetFunction::GetDescriptor (void *pBuffer, unsigned nSize) const
{
	if (!m_bValid || !pBuffer)
	{
		return 0;
	}
	unsigned nFeatureSize = 8 + m_Config.nChannels;
	unsigned nControlSize = 9 + 12 + nFeatureSize + 9;
	unsigned nRequired = 9 + nControlSize + 9;
	for (unsigned i = 0; i < m_Config.nAlternates; i++)
	{
		nRequired += 9 + 7 + 8 + 3*m_Config.Alternates[i].nRates + 9 + 7;
	}
	if (nSize < nRequired || nRequired > MaxDescriptorSize)
	{
		return 0;
	}
	CDescriptorWriter W ((u8 *) pBuffer);
	// AudioControl interface and header.
	W.Byte (9); W.Byte (DESCRIPTOR_INTERFACE); W.Byte (m_Config.nControlInterface);
	W.Byte (0); W.Byte (0); W.Byte (1); W.Byte (1);
	W.Byte (USB_PROTO_AUDIO_VER_100); W.Byte (0);
	W.Byte (9); W.Byte (DESCRIPTOR_CS_INTERFACE); W.Byte (USB_AUDIO_CTL_IFACE_SUBTYPE_HEADER);
	W.Word (USB_AUDIO_CTL_IFACE_BCDADC_100);
	W.Word (nControlSize); W.Byte (1); W.Byte (m_Config.nStreamingInterface);
	// USB streaming Input Terminal, master mute/volume Feature Unit, speaker OT.
	W.Byte (12); W.Byte (DESCRIPTOR_CS_INTERFACE);
	W.Byte (USB_AUDIO_CTL_IFACE_SUBTYPE_INPUT_TERMINAL); W.Byte (m_Config.nInputTerminal);
	W.Word (USB_AUDIO_TERMINAL_TYPE_USB_STREAMING); W.Byte (0); W.Byte (m_Config.nChannels);
	W.Word (m_Config.wChannelConfig); W.Byte (0); W.Byte (0);
	W.Byte (nFeatureSize); W.Byte (DESCRIPTOR_CS_INTERFACE);
	W.Byte (USB_AUDIO_CTL_IFACE_SUBTYPE_FEATURE_UNIT); W.Byte (m_Config.nFeatureUnit);
	W.Byte (m_Config.nInputTerminal); W.Byte (1);
	W.Byte ((1 << (USB_AUDIO_FU_MUTE_CONTROL-1)) | (1 << (USB_AUDIO_FU_VOLUME_CONTROL-1)));
	for (unsigned i = 0; i < m_Config.nChannels; i++)
	{
		W.Byte (0);
	}
	W.Byte (0);
	W.Byte (9); W.Byte (DESCRIPTOR_CS_INTERFACE);
	W.Byte (USB_AUDIO_CTL_IFACE_SUBTYPE_OUTPUT_TERMINAL); W.Byte (m_Config.nOutputTerminal);
	W.Word (USB_AUDIO_TERMINAL_TYPE_SPEAKER); W.Byte (0); W.Byte (m_Config.nFeatureUnit); W.Byte (0);
	// Zero-bandwidth alternate is mandatory.
	W.Byte (9); W.Byte (DESCRIPTOR_INTERFACE); W.Byte (m_Config.nStreamingInterface);
	W.Byte (0); W.Byte (0); W.Byte (1); W.Byte (2);
	W.Byte (USB_PROTO_AUDIO_VER_100); W.Byte (0);
	for (unsigned i = 0; i < m_Config.nAlternates; i++)
	{
		const TAlternate &Alt = m_Config.Alternates[i];
		unsigned nBytes = GetSubframeSize (Alt.Format);
		W.Byte (9); W.Byte (DESCRIPTOR_INTERFACE); W.Byte (m_Config.nStreamingInterface);
		W.Byte (i+1); W.Byte (1); W.Byte (1); W.Byte (2);
		W.Byte (USB_PROTO_AUDIO_VER_100); W.Byte (0);
		W.Byte (7); W.Byte (DESCRIPTOR_CS_INTERFACE); W.Byte (USB_AUDIO_STREAMING_GENERAL);
		W.Byte (m_Config.nInputTerminal);
		W.Byte (0);
		unsigned nTag = USB_AUDIO_FORMAT_TAG_PCM;
		if (Alt.Format == U8)
		{
			nTag = USB_AUDIO_FORMAT_TAG_PCM8;
		}
		else if (Alt.Format == Float32)
		{
			nTag = USB_AUDIO_FORMAT_TAG_IEEE_FLOAT;
		}
		W.Word (nTag);
		W.Byte (8 + 3*Alt.nRates); W.Byte (DESCRIPTOR_CS_INTERFACE);
		W.Byte (USB_AUDIO_FORMAT_TYPE); W.Byte (USB_AUDIO_FORMAT_TYPE_I);
		W.Byte (m_Config.nChannels); W.Byte (nBytes); W.Byte (8*nBytes); W.Byte (Alt.nRates);
		for (unsigned j = 0; j < Alt.nRates; j++)
		{
			W.Triple (Alt.Rates[j]);
		}
		W.Byte (9); W.Byte (DESCRIPTOR_ENDPOINT); W.Byte (m_Config.nEndpoint);
		W.Byte (9); // Isochronous, adaptive OUT; no explicit feedback endpoint
		W.Word (PacketSize (i)); W.Byte (1); W.Byte (0); W.Byte (0);
		W.Byte (7); W.Byte (DESCRIPTOR_CS_ENDPOINT); W.Byte (1); W.Byte (1);
		W.Byte (0); W.Word (0); // No advertised hardware lock delay
	}
	return W.Length ();
}

void CUSBAudioGadgetFunction::Changed (void)
{
	++m_nRevision;
	m_bReady = FALSE;
	m_nHead = m_nTail = m_nCount = 0;
}

unsigned CUSBAudioGadgetFunction::NearestRate (unsigned nAlternate, unsigned nRate) const
{
	const TAlternate &Alt = m_Config.Alternates[nAlternate];
	unsigned nBest = Alt.Rates[0];
	unsigned nDistance = nRate > nBest ? nRate-nBest : nBest-nRate;
	for (unsigned i = 1; i < Alt.nRates; i++)
	{
		unsigned nCandidate = Alt.Rates[i];
		unsigned nDelta = nRate > nCandidate ? nRate-nCandidate : nCandidate-nRate;
		if (nDelta < nDistance)
		{
			nBest = nCandidate;
			nDistance = nDelta;
		}
	}
	return nBest;
}

boolean CUSBAudioGadgetFunction::SetInterface (unsigned nInterface, unsigned nAlternate)
{
	if (!m_bValid)
	{
		return FALSE;
	}
	m_Lock.Acquire ();
	boolean bOK = m_bConfigured && !m_bSuspended;
	if (nInterface == m_Config.nControlInterface)
	{
		bOK = bOK && nAlternate == 0;
	}
	else if (nInterface == m_Config.nStreamingInterface && nAlternate <= m_Config.nAlternates)
	{
		if (bOK)
		{
			m_nAlternate = nAlternate;
			if (nAlternate)
			{
				m_nRate = NearestRate (nAlternate-1, m_nRate);
			}
			Changed ();
		}
	}
	else
	{
		bOK = FALSE;
	}
	m_Lock.Release ();
	return bOK;
}

int CUSBAudioGadgetFunction::GetInterface (unsigned nInterface)
{
	m_Lock.Acquire ();
	int nResult = -1;
	if (m_bValid && m_bConfigured)
	{
		if (nInterface == m_Config.nControlInterface)
		{
			nResult = 0;
		}
		else if (nInterface == m_Config.nStreamingInterface)
		{
			nResult = m_nAlternate;
		}
	}
	m_Lock.Release ();
	return nResult;
}

int CUSBAudioGadgetFunction::ControlRequest (const TSetupData *pSetup, u8 *pData)
{
	if (!m_bValid || !pSetup || !pData)
	{
		return -1;
	}
	m_Lock.Acquire ();
	int nResult = -1;
	boolean bGet = pSetup->bmRequestType & 0x80;
	unsigned nType = pSetup->bmRequestType;
	unsigned nRequest = pSetup->bRequest;
	if (!m_bConfigured || m_bSuspended)
	{
	}
	else if ((nType == (REQUEST_CLASS | REQUEST_TO_ENDPOINT) || nType == (REQUEST_IN | REQUEST_CLASS | REQUEST_TO_ENDPOINT)) && pSetup->wIndex == m_Config.nEndpoint
		 && pSetup->wValue == (USB_AUDIO_CS_SAM_FREQ_CONTROL << 8) && pSetup->wLength == 3)
	{
		unsigned nAlt = m_nAlternate ? m_nAlternate-1 : 0;
		const TAlternate &Alt = m_Config.Alternates[nAlt];
		if (!bGet && nRequest == USB_AUDIO_REQ_SET_CUR)
		{
			unsigned nRate = pData[0] | (unsigned) pData[1]<<8 | (unsigned) pData[2]<<16;
			// UAC1 5.2.3.2.3.1 requires rounding discrete frequency requests.
			unsigned nRounded = NearestRate (nAlt, nRate);
			if (nRounded != m_nRate)
			{
				m_nRate = nRounded;
				Changed ();
			}
			nResult = 0;
		}
		else if (bGet)
		{
			unsigned nValue = m_nRate;
			if (nRequest == USB_AUDIO_REQ_GET_MIN)
			{
				nValue = Alt.Rates[0];
			}
			else if (nRequest == USB_AUDIO_REQ_GET_MAX)
			{
				nValue = Alt.Rates[Alt.nRates-1];
			}
			else if (nRequest == USB_AUDIO_REQ_GET_RES)
			{
				nValue = 1;
			}
			if (nRequest >= USB_AUDIO_REQ_GET_CUR && nRequest <= USB_AUDIO_REQ_GET_RES)
			{
				pData[0] = (u8) nValue;
				pData[1] = (u8) (nValue>>8);
				pData[2] = (u8) (nValue>>16);
				nResult = 3;
			}
		}
	}
	else if ((nType == (REQUEST_CLASS | REQUEST_TO_INTERFACE) || nType == (REQUEST_IN | REQUEST_CLASS | REQUEST_TO_INTERFACE))
		 && pSetup->wIndex == ((m_Config.nFeatureUnit<<8) | m_Config.nControlInterface))
	{
		if (pSetup->wValue == (USB_AUDIO_FU_MUTE_CONTROL << 8) && pSetup->wLength == 1)
		{
			if (bGet && nRequest == USB_AUDIO_REQ_GET_CUR)
			{
				pData[0] = m_bMute;
				nResult = 1;
			}
			else if (!bGet && nRequest == USB_AUDIO_REQ_SET_CUR && pData[0] <= 1)
			{
				m_bMute = pData[0];
				m_bControlsPending = TRUE;
				nResult = 0;
			}
		}
		else if (pSetup->wValue == (USB_AUDIO_FU_VOLUME_CONTROL << 8) && pSetup->wLength == 2)
		{
			if (!bGet && nRequest == USB_AUDIO_REQ_SET_CUR)
			{
				int nVolume = pData[0] | (unsigned) pData[1]<<8;
				if (nVolume & 0x8000)
				{
					nVolume -= 0x10000;
				}
				if (nVolume >= VolumeMinimum && nVolume <= 0 && nVolume % VolumeResolution == 0)
				{
					m_nVolume = nVolume;
					m_bControlsPending = TRUE;
					nResult = 0;
				}
			}
			else if (bGet && nRequest >= USB_AUDIO_REQ_GET_CUR && nRequest <= USB_AUDIO_REQ_GET_RES)
			{
				int nVolume = nRequest == USB_AUDIO_REQ_GET_MIN ? VolumeMinimum
					: nRequest == USB_AUDIO_REQ_GET_MAX ? 0
					: nRequest == USB_AUDIO_REQ_GET_RES ? VolumeResolution : m_nVolume;
				pData[0] = (u8) nVolume;
				pData[1] = (u8) ((unsigned) nVolume>>8);
				nResult = 2;
			}
		}
	}
	m_Lock.Release ();
	return nResult;
}

void CUSBAudioGadgetFunction::SetConfigured (boolean bConfigured)
{
	m_Lock.Acquire ();
	m_bConfigured = bConfigured;
	m_bSuspended = FALSE;
	m_nAlternate = 0;
	Changed ();
	m_Lock.Release ();
}

void CUSBAudioGadgetFunction::SetSuspended (boolean bSuspended)
{
	m_Lock.Acquire ();
	m_bSuspended = bSuspended;
	Changed ();
	m_Lock.Release ();
}

void CUSBAudioGadgetFunction::Receive (const void *pData, unsigned nLength)
{
	m_Lock.Acquire ();
	if (!m_bReady || !m_nAlternate || !nLength)
	{
		if (nLength)
		{
			++m_Statistics.nDropped;
		}
	}
	else if (!pData || nLength > GetPacketSize (m_Stream.nSampleRate,
						 m_Stream.nChannels, m_Stream.Format)
		 || nLength % (m_Stream.nChannels * GetSubframeSize (m_Stream.Format)))
	{
		++m_Statistics.nMalformed;
	}
	else
	{
		++m_Statistics.nPackets;
		if (m_nCount == QueuePackets)
		{
			// Prefer bounded latency: discard oldest packet, never overwrite DMA.
			m_nTail = (m_nTail+1) % QueuePackets;
			--m_nCount;
			++m_Statistics.nDropped;
		}
		TPacket &Packet = m_Queue[m_nHead];
		Packet.nLength = nLength;
		memcpy (Packet.Data, pData, nLength);
		m_nHead = (m_nHead+1) % QueuePackets;
		++m_nCount;
	}
	m_Lock.Release ();
}

void CUSBAudioGadgetFunction::Process (void)
{
	if (!m_bValid)
	{
		return;
	}
	m_Lock.Acquire ();
	unsigned nRevision = m_nRevision;
	boolean bTransition = nRevision != m_nAppliedRevision;
	boolean bActive = m_bConfigured && !m_bSuspended && m_nAlternate;
	TStream Stream = m_Stream;
	if (bActive)
	{
		Stream = {m_Config.Alternates[m_nAlternate-1].Format, m_Config.nChannels, m_nRate};
	}
	boolean bControls = m_bControlsPending;
	boolean bMute = m_bMute;
	int nVolume = m_nVolume;
	m_bControlsPending = FALSE;
	m_Lock.Release ();

	if (bTransition)
	{
		m_pSink->Stop ();
		boolean bOK = !bActive || m_pSink->Configure (Stream);
		m_Lock.Acquire ();
		// A reset or another request may arrive during the hardware callback.
		boolean bCurrent = nRevision == m_nRevision;
		if (bCurrent)
		{
			m_nAppliedRevision = nRevision;
			m_Stream = Stream;
			m_bReady = bOK && bActive;
			++m_Statistics.nTransitions;
			if (!bOK)
			{
				++m_Statistics.nFailures;
			}
		}
		m_Lock.Release ();
		if (!bOK || !bCurrent)
		{
			m_pSink->Stop ();
		}
	}
	if (bControls || bTransition)
	{
		m_pSink->SetControls (bMute, nVolume);
	}
	for (unsigned i = 0; i < QueuePackets; i++)
	{
		TPacket Packet;
		m_Lock.Acquire ();
		if (!m_bReady || nRevision != m_nRevision || !m_nCount)
		{
			m_Lock.Release ();
			break;
		}
		const TPacket &Queued = m_Queue[m_nTail];
		Packet.nLength = Queued.nLength;
		memcpy (Packet.Data, Queued.Data, Packet.nLength);
		m_nTail = (m_nTail+1) % QueuePackets;
		--m_nCount;
		m_Lock.Release ();
		if (!m_pSink->Write (Packet.Data,
			Packet.nLength / (Stream.nChannels * GetSubframeSize (Stream.Format)), Stream))
		{
			m_Lock.Acquire ();
			++m_Statistics.nDropped;
			m_Lock.Release ();
		}
	}
}

void CUSBAudioGadgetFunction::GetStatistics (TStatistics *pStatistics)
{
	if (!pStatistics)
	{
		return;
	}
	m_Lock.Acquire ();
	*pStatistics = m_Statistics;
	m_Lock.Release ();
}
