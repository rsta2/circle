// SPDX-License-Identifier: GPL-3.0-or-later
//
// Circle - A C++ bare metal environment for Raspberry Pi
//
#include <circle/usb/gadget/usbaudiogadgetcapture.h>
#include <circle/usb/usbaudio.h>
#include <circle/util.h>
#include <assert.h>

using Source = CUSBAudioCaptureSource;
static const unsigned PacketBytes = Source::MaxFrames * Source::Channels * sizeof (s16);

TUSBAudioCaptureDescriptors::TUSBAudioCaptureDescriptors (
	u8 nInterface, u8 nEndpoint, u8 nInputTerminal, u8 nOutputTerminal, u16 usTerminalType,
	TClock Clock)
:	Endpoint {7, DESCRIPTOR_ENDPOINT, (u8) (0x80 | nEndpoint),
		  (u8) (Clock == Asynchronous ? 5 : 13),
		  (u16) (Clock == Asynchronous ? PacketBytes : Source::NominalFrames * 4), 1},
	Terminals {
		12, DESCRIPTOR_CS_INTERFACE, USB_AUDIO_CTL_IFACE_SUBTYPE_INPUT_TERMINAL, nInputTerminal,
		(u8) usTerminalType, (u8) (usTerminalType >> 8), 0, 2, 3, 0, 0, 0,
		9, DESCRIPTOR_CS_INTERFACE, USB_AUDIO_CTL_IFACE_SUBTYPE_OUTPUT_TERMINAL, nOutputTerminal,
		(u8) USB_AUDIO_TERMINAL_TYPE_USB_STREAMING,
		USB_AUDIO_TERMINAL_TYPE_USB_STREAMING >> 8, 0, nInputTerminal, 0},
	Streaming {
		9, DESCRIPTOR_INTERFACE, nInterface, 0, 0, 1, 2, 0, 0,
		9, DESCRIPTOR_INTERFACE, nInterface, 1, 1, 1, 2, 0, 0,
		7, DESCRIPTOR_CS_INTERFACE, USB_AUDIO_STREAMING_GENERAL, nOutputTerminal,
		1, USB_AUDIO_FORMAT_TAG_PCM, 0,
		11, DESCRIPTOR_CS_INTERFACE, USB_AUDIO_FORMAT_TYPE, USB_AUDIO_FORMAT_TYPE_I,
		2, 2, 16, 1, 0x80, 0xBB, 0,
		9, DESCRIPTOR_ENDPOINT, Endpoint.bEndpointAddress, Endpoint.bmAttributes,
		(u8) Endpoint.wMaxPacketSize, 0, 1, 0, 0,
		7, DESCRIPTOR_CS_ENDPOINT, 1, 0, 0, 0, 0}
{
	assert (nEndpoint && nEndpoint <= 7);
	assert (nInputTerminal && nOutputTerminal && nInputTerminal != nOutputTerminal);
}

void CUSBAudioCaptureBuffer::SetRecording (boolean bActive)
{
	m_Lock.Acquire ();
	m_bRecording = bActive;
	m_bPrimed = FALSE;
	if (bActive)
	{
		m_nRead = m_nWrite;
		m_nPackets = m_nProduced = m_nSent = m_nOverruns = m_nUnderruns = 0;
	}
	m_Lock.Release ();
}

void CUSBAudioCaptureBuffer::WriteFrames (const s16 *pSamples, unsigned nFrames)
{
	assert (pSamples || !nFrames);
	// Limit IRQ-off time even for callers submitting a large block.
	while (nFrames)
	{
		unsigned nBlock = nFrames > 64 ? 64 : nFrames;
		m_Lock.Acquire ();
		if (!m_bRecording)
		{
			m_Lock.Release ();
			return;
		}
		m_nProduced += nBlock;
		for (unsigned i = 0; i < nBlock; ++i)
		{
			if (m_nWrite - m_nRead == Capacity)
			{
				++m_nRead;
				++m_nOverruns;
			}
			unsigned nIndex = (m_nWrite++ % Capacity) * Channels;
			m_Buffer[nIndex] = *pSamples++;
			m_Buffer[nIndex+1] = *pSamples++;
		}
		m_Lock.Release ();
		nFrames -= nBlock;
	}
}

unsigned CUSBAudioCaptureBuffer::ReadPacket (s16 *pSamples)
{
	assert (pSamples);
	m_Lock.Acquire ();
	unsigned nAvailable = m_nWrite - m_nRead;
	if (m_bRecording && !m_bPrimed && nAvailable >= Reserve)
	{
		// The host may delay its first IN token. Discard excess startup
		// backlog before using queue depth to estimate clock drift.
		m_nRead = m_nWrite - Reserve;
		nAvailable = Reserve;
		m_bPrimed = TRUE;
	}
	unsigned nFrames = NominalFrames;
	if (++m_nPackets % 8 == 0)
	{
		if (nAvailable > Reserve + NominalFrames) ++nFrames;
		else if (nAvailable < Reserve - NominalFrames) --nFrames;
	}
	if (m_bPrimed && nAvailable >= nFrames)
	{
		for (unsigned i = 0; i < nFrames; ++i)
		{
			unsigned nIndex = (m_nRead++ % Capacity) * Channels;
			*pSamples++ = m_Buffer[nIndex];
			*pSamples++ = m_Buffer[nIndex+1];
		}
		m_nSent += nFrames;
	}
	else
	{
		if (m_bPrimed)
		{
			++m_nUnderruns;
			m_bPrimed = FALSE;
		}
		nFrames = NominalFrames;
		memset (pSamples, 0, nFrames * Channels * sizeof (s16));
	}
	m_Lock.Release ();
	return nFrames;
}

void CUSBAudioCaptureBuffer::GetStatistics (TStatistics *pStatistics)
{
	assert (pStatistics);
	m_Lock.Acquire ();
	*pStatistics = {m_nProduced, m_nSent, m_nWrite - m_nRead, m_nOverruns, m_nUnderruns};
	m_Lock.Release ();
}

CUSBAudioCaptureEndpoint::CUSBAudioCaptureEndpoint (const TUSBAudioCaptureDescriptors &Descriptors,
						    CDWUSBGadget *pGadget, Source *pSource)
:	CDWUSBGadgetEndpoint (&Descriptors.Endpoint, pGadget),
	m_pSource (pSource),
	m_nInterface (Descriptors.Streaming[2]),
	m_nMaxFrames (Descriptors.Endpoint.wMaxPacketSize / (Source::Channels * sizeof (s16)))
{
	assert (pSource);
}

boolean CUSBAudioCaptureEndpoint::SetInterface (u16 nInterface, u16 nAlternate)
{
	if (nInterface != m_nInterface || nAlternate > 1 || (nAlternate && !m_bConfigured))
	{
		return FALSE;
	}
	boolean bEnable = nAlternate != 0;
	if (bEnable != m_bStreaming)
	{
		m_bStreaming = bEnable;
		m_pSource->SetRecording (bEnable);
		if (bEnable)
		{
			Arm ();
		}
		else if (IsTransferActive ())
		{
			CancelTransfer ();
		}
	}
	return TRUE;
}

int CUSBAudioCaptureEndpoint::GetInterface (u16 nInterface) const
{
	return nInterface == m_nInterface ? (int) m_bStreaming : -1;
}

void CUSBAudioCaptureEndpoint::OnActivate (void)
{
	m_bConfigured = TRUE;
}

void CUSBAudioCaptureEndpoint::OnDeactivate (void)
{
	SetInterface (m_nInterface, 0);
	m_bConfigured = FALSE;
}

void CUSBAudioCaptureEndpoint::OnSuspend (void)
{
	OnDeactivate ();
}

void CUSBAudioCaptureEndpoint::OnUSBReset (void)
{
	if (m_bStreaming) m_pSource->SetRecording (FALSE);
	m_bStreaming = m_bConfigured = FALSE;
	CDWUSBGadgetEndpoint::OnUSBReset ();
}

void CUSBAudioCaptureEndpoint::OnTransferComplete (boolean bIn, size_t)
{
	if (bIn && m_bStreaming) Arm ();
}

void CUSBAudioCaptureEndpoint::Arm (void)
{
	unsigned nFrames = m_pSource->ReadPacket (m_Samples);
	if (   !nFrames || nFrames > m_nMaxFrames
	    || (m_nMaxFrames == Source::NominalFrames && nFrames != m_nMaxFrames))
	{
		nFrames = Source::NominalFrames;
		memset (m_Samples, 0, nFrames * Source::Channels * sizeof (s16));
	}
	BeginTransfer (TransferDataIn, m_Samples, nFrames * Source::Channels * sizeof (s16));
}
