//
// usbaudiostreaming.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2022  R. Stange <rsta2@o2online.de>
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

/*
USB audio streaming architecture:

The USB audio streaming support in Circle is based on the following components
(top -> down, 1 instance each per hardware device, if not specified differently):

------------------------------	--------------------------------------------------
Component / class name		Function
Source files
Device name
USB interface(s)
------------------------------	--------------------------------------------------
Application with sound		creates and calls instance of CUSBSoundBaseDevice,
				gets pointer to instance of CUSBSoundController
				from CUSBSoundBaseDevice and calls it
------------------------------	--------------------------------------------------
CUSBSoundBaseDevice		creates and probes instance of CUSBSoundController,
sound/usbsoundbasedevice.*	calls instance(s) of CUSBAudioStreamingDevice,
"sndusb[N]" (N > 0)		maintains sound samples in sound queue(s)
none				for TX and RX
------------------------------	--------------------------------------------------
CUSBSoundController		calls instance of CUSBSoundBaseDevice, which did
sound/usbsoundcontroller.*	create it,
none				calls instance of CUSBAudioStreamingDevice to get
none				device information and to set controls (e.g. mute)
------------------------------	--------------------------------------------------
CUSBAudioStreamingDevice	M instances per hardware device (1 instance for RX,
usb/usbaudiostreaming.*		M-1 instances for TX), configures USB endpoints,
"uaudioN-M"			manages per packet data transfer to endpoints and
"int1-2-0", "int1-2-20"		synchronization, calls instance of
				CUSBAudioControlDevice to get device information
				of CUSBSoundBaseDevice on completion of transfers,
				calls USB HCI driver to transfer data
------------------------------	--------------------------------------------------
CUSBAudioControlDevice		parses and maintains topology of the audio device,
usb/audiocontrol.*		owns 1 instance of CUSBAudioFunctionTopology
"uaudioctlN"
"int1-1-0", "int1-1-20"
------------------------------	--------------------------------------------------
CUSBAudioFunctionTopology	member of CUSBAudioControlDevice
usb/usbaudiofunctopology.*
none
none
------------------------------	--------------------------------------------------
USB HCI driver			creates instance(s) of CUSBAudioStreamingDevice
				and instance of CUSBAudioControlDevice, calls
				them for configuration and on completion of USB
				data transfers
------------------------------	--------------------------------------------------

Topology of an USB audio streaming device:

The topology of components of an USB audio streaming device (called a "device")
varies and can be complex. To be able to support such devices in Circle, the
following assumptions were made:

* An device can have 1 USB audio streaming interface with the supported audio
  parameters for input of audio data (RX) and N USB interfaces with the supported
  audio parameters for output (TX).

* Only USB audio streaming interfaces which support the following audio
  parameters will be used: 16-bit signed audio samples, 2 (Stereo) or 1 (Mono,
  RX only) channels

* TX: An USB audio streaming interface input terminal (IT) is connected
  downstream to one output terminal (e.g. Speaker) via one optional feature unit,
  with the audio controls mute (channel global) and/or volume (channel global and
  optionally per channel).

* RX: An USB audio streaming interface output terminal (OT) is connected
  upstream to N input terminals (e.g. microphone, line-in) via one selector unit,
  and one optional feature unit per input terminal, with the same audio controls.

References:

Universal Serial Bus Device Class Definition for Audio Devices,
Release 1.0 and 2.0, https://usb.org/documents
*/

#include <circle/usb/usbaudiostreaming.h>
#include <circle/usb/usbaudiofunctopology.h>
#include <circle/usb/usbaudio.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/devicenameservice.h>
#include <circle/debug.h>
#include <circle/util.h>
#include <assert.h>

// supported format
#define CHANNELS		2		// Stereo
#define SUBFRAME_SIZE		2		// 16-bit signed
#define CHUNK_FREQUENCY		1000		// per second

// convert 3-byte sample rate value to an unsigned
#define RATE2UNSIGNED(rate)	(  (unsigned) (rate)[0]		\
				 | (unsigned) (rate)[1] << 8	\
				 | (unsigned) (rate)[2] << 16)

static const char DeviceNamePattern[] = "uaudio%u-%u";

CUSBAudioStreamingDevice::CUSBAudioStreamingDevice (CUSBFunction *pFunction)
:	CUSBFunction (pFunction),
	m_pEndpointData (nullptr),
	m_pEndpointSync (nullptr),
	m_nChannels (0),
	m_nTerminals (1),
	m_nActiveTerminal (0),
	m_nSampleRate (0),
	m_nChunkSizeBytes (0),
	m_nPacketsPerChunk (0),
	m_bSyncEPActive (FALSE),
	m_nSyncAccu (0),
	m_uchClockSourceID (USB_AUDIO_UNDEFINED_UNIT_ID),
	m_uchSelectorUnitID (USB_AUDIO_UNDEFINED_UNIT_ID),
	From ("uaudio")
{
	for (unsigned i = 0; i < MaxTerminals; i++)
	{
		m_uchFeatureUnitID[i] = USB_AUDIO_UNDEFINED_UNIT_ID;
	}

	memset (&m_DeviceInfo, 0, sizeof m_DeviceInfo);
}

CUSBAudioStreamingDevice::~CUSBAudioStreamingDevice (void)
{
	if (m_DeviceName.GetLength ())
	{
		CDeviceNameService::Get ()->RemoveDevice (m_DeviceName, FALSE);
	}

	delete m_pEndpointSync;
	m_pEndpointSync = nullptr;

	delete m_pEndpointData;
	m_pEndpointData = nullptr;
}

boolean CUSBAudioStreamingDevice::Initialize (void)
{
	if (!CUSBFunction::Initialize ())
	{
		return FALSE;
	}

	return GetNumEndpoints () >= 1;		// ignore no-endpoint interfaces
}

boolean CUSBAudioStreamingDevice::Configure (void)
{
	assert (GetNumEndpoints () >= 1);

	m_bVer200 = GetInterfaceProtocol () == USB_PROTO_AUDIO_VER_200;

	CUSBAudioStreamingInterfaceDescriptor *pGeneralDesc;
	while ((pGeneralDesc = (CUSBAudioStreamingInterfaceDescriptor *)
					GetDescriptor (DESCRIPTOR_CS_INTERFACE)) != nullptr)
	{
		if (pGeneralDesc->bDescriptorSubtype == USB_AUDIO_STREAMING_GENERAL)
		{
			break;
		}
	}

	if (!pGeneralDesc)
	{
		LOGWARN ("AS_GENERAL descriptor expected");

		return FALSE;
	}

	TUSBAudioTypeIFormatTypeDescriptor *pFormatTypeDesc =
		(TUSBAudioTypeIFormatTypeDescriptor *) GetDescriptor (DESCRIPTOR_CS_INTERFACE);
	if (   !pFormatTypeDesc
	    || pFormatTypeDesc->bDescriptorSubtype != USB_AUDIO_FORMAT_TYPE)
	{
		LOGWARN ("FORMAT_TYPE descriptor expected");

		return FALSE;
	}

	TUSBAudioEndpointDescriptor *pEndpointDesc;
	pEndpointDesc = (TUSBAudioEndpointDescriptor *) GetDescriptor (DESCRIPTOR_ENDPOINT);
	if (   !pEndpointDesc
	    || (pEndpointDesc->bmAttributes & 0x33) != 0x01) // Isochronous, Data
	{
		LOGDBG ("Isochronous data EP expected");

		return FALSE;
	}

	m_bIsOutput = (pEndpointDesc->bEndpointAddress & 0x80) == 0x00;

#if RASPPI <= 3
	if (!m_bIsOutput)
	{
		LOGWARN ("USB audio input is not supported");

		return FALSE;
	}
#endif

	if (pEndpointDesc->bInterval != 1)			// TODO
	{
		LOGWARN ("Unsupported EP timing (%u)", (unsigned) pEndpointDesc->bInterval);

		return FALSE;
	}

	if (!m_bVer200)
	{
		if (   pFormatTypeDesc->bFormatType           != USB_AUDIO_FORMAT_TYPE_I
		    || pFormatTypeDesc->Ver100.bNrChannels    == 0
		    || pFormatTypeDesc->Ver100.bNrChannels    > CHANNELS
		    || pFormatTypeDesc->Ver100.bSubframeSize  != SUBFRAME_SIZE
		    || pFormatTypeDesc->Ver100.bBitResolution != SUBFRAME_SIZE*8)
		{
			LOGWARN ("Unsupported audio format");
#ifndef NDEBUG
			debug_hexdump (pFormatTypeDesc, pFormatTypeDesc->bLength, From);
#endif

			return FALSE;
		}

		m_nChannels = pFormatTypeDesc->Ver100.bNrChannels;
	}
	else
	{
		if (   pFormatTypeDesc->bFormatType           != USB_AUDIO_FORMAT_TYPE_I
		    || pFormatTypeDesc->Ver200.bSubslotSize   != SUBFRAME_SIZE
		    || pFormatTypeDesc->Ver200.bBitResolution != SUBFRAME_SIZE*8
		    || pGeneralDesc->Ver200.bNrChannels       == 0
		    || pGeneralDesc->Ver200.bNrChannels       > CHANNELS)
		{
			LOGWARN ("Unsupported audio format (chans %u)",
				 (unsigned) pGeneralDesc->Ver200.bNrChannels);
#ifndef NDEBUG
			debug_hexdump (pFormatTypeDesc, pFormatTypeDesc->bLength, From);
#endif

			return FALSE;
		}

		m_nChannels = pGeneralDesc->Ver200.bNrChannels;
	}

	if (   m_bIsOutput
	    && m_nChannels == 1)
	{
		LOGWARN ("Mono output is not supported");

		return FALSE;
	}

	if (   m_bIsOutput
	    && (pEndpointDesc->bmAttributes & 0x0C) == 0x04)		// Asynchronous sync
	{
		TUSBAudioEndpointDescriptor *pEndpointInDesc;
		pEndpointInDesc = (TUSBAudioEndpointDescriptor *) GetDescriptor (DESCRIPTOR_ENDPOINT);
		if (   !pEndpointInDesc
		    || (pEndpointInDesc->bmAttributes     & 0x3F) != 0x11  // Isochronous, Feedback
		    || (pEndpointInDesc->bEndpointAddress & 0x80) != 0x80) // Input EP
		{
			LOGWARN ("Isochronous feedback input EP expected");

			return FALSE;
		}

		m_pEndpointSync = new CUSBEndpoint (GetDevice (), (TUSBEndpointDescriptor *) pEndpointInDesc);
		assert (m_pEndpointSync != 0);
	}

	m_bSynchronousSync = (pEndpointDesc->bmAttributes & 0x0C) == 0x0C;

	m_pEndpointData = new CUSBEndpoint (GetDevice (), (TUSBEndpointDescriptor *) pEndpointDesc);
	assert (m_pEndpointData != 0);

	if (!CUSBFunction::Configure ())
	{
		LOGWARN ("Cannot set interface");

		return FALSE;
	}

	// Interface of audio control device is the first in configuration descriptor,
	// so that the respective function has the index 0.
	CUSBFunction *pFunction = GetDevice ()->GetFunction (0);
	CUSBAudioControlDevice *pControlDevice = (CUSBAudioControlDevice *) pFunction;
	if (   !pFunction
	    || pControlDevice->GetInterfaceClass ()    != 1
	    || pControlDevice->GetInterfaceSubClass () != 1)
	{
		LOGWARN ("Associated control device not found");

		return FALSE;
	}

	m_DeviceInfo.IsOutput = m_bIsOutput;
	m_DeviceInfo.NumChannels = m_nChannels;

	u8 uchTerminalLink = USB_AUDIO_UNDEFINED_UNIT_ID;

	if (!m_bVer200)
	{
		// fetch format info from descriptor
		if (pFormatTypeDesc->Ver100.bSamFreqType == 0)
		{
			// continuous range
			m_DeviceInfo.SampleRateRanges = 1;
			m_DeviceInfo.SampleRateRange[0].Min =
				RATE2UNSIGNED (pFormatTypeDesc->Ver100.tSamFreq[0]);
			m_DeviceInfo.SampleRateRange[0].Max =
				RATE2UNSIGNED (pFormatTypeDesc->Ver100.tSamFreq[1]);
		}
		else
		{
			// discrete sample rates
			unsigned nSampleRates = pFormatTypeDesc->Ver100.bSamFreqType;
			if (nSampleRates > MaxSampleRatesRanges)
			{
				nSampleRates = MaxSampleRatesRanges;
			}
			m_DeviceInfo.SampleRateRanges = nSampleRates;

			for (unsigned i = 0; i < nSampleRates; i++)
			{
				m_DeviceInfo.SampleRateRange[i].Min =
				m_DeviceInfo.SampleRateRange[i].Max =
					RATE2UNSIGNED (pFormatTypeDesc->Ver100.tSamFreq[i]);
			}
		}

		uchTerminalLink = pGeneralDesc->Ver100.bTerminalLink;
	}
	else
	{
		// request clock source ID for this Terminal
		m_uchClockSourceID =
			pControlDevice->GetClockSourceID (pGeneralDesc->Ver200.bTerminalLink);
		if (m_uchClockSourceID == USB_AUDIO_UNDEFINED_UNIT_ID)
		{
			LOGWARN ("Associated clock source not found (%u)",
				 (unsigned) pGeneralDesc->Ver200.bTerminalLink);

			return FALSE;
		}

		// fetch supported sampling frequency ranges from clock source,
		// number of ranges first
		DMA_BUFFER (u16, NumSubRanges, 1);
		if (GetHost ()->ControlMessage (GetEndpoint0 (),
						REQUEST_IN | REQUEST_CLASS | REQUEST_TO_INTERFACE,
						USB_AUDIO_REQ_RANGE,
						USB_AUDIO_CS_SAM_FREQ_CONTROL << 8,
						m_uchClockSourceID << 8,
						NumSubRanges, 2) < 0)
		{
			LOGWARN ("Cannot get number of sampling frequency subranges");

			return FALSE;
		}

		// now that we know the number of ranges, request the whole parameter block
		unsigned nSampleRates = NumSubRanges[0];
		unsigned nBufferSize = 2 + 12*nSampleRates;
		DMA_BUFFER (u8, RangesBuffer, nBufferSize);
		if (GetHost ()->ControlMessage (GetEndpoint0 (),
						REQUEST_IN | REQUEST_CLASS | REQUEST_TO_INTERFACE,
						USB_AUDIO_REQ_RANGE,
						USB_AUDIO_CS_SAM_FREQ_CONTROL << 8,
						m_uchClockSourceID << 8,
						RangesBuffer, nBufferSize) < 0)
		{
			LOGWARN ("Cannot get sampling frequency ranges");

			return FALSE;
		}

		// fill in the m_DeviceInfo struct
		if (nSampleRates > MaxSampleRatesRanges)
		{
			nSampleRates = MaxSampleRatesRanges;
		}
		m_DeviceInfo.SampleRateRanges = nSampleRates;

		u32 *pFreq = (u32 *) (RangesBuffer+2);
		for (unsigned i = 0; i < nSampleRates; i++)
		{
			m_DeviceInfo.SampleRateRange[i].Min = *pFreq++;
			m_DeviceInfo.SampleRateRange[i].Max = *pFreq++;
			m_DeviceInfo.SampleRateRange[i].Resolution = *pFreq++;
		}

		uchTerminalLink = pGeneralDesc->Ver200.bTerminalLink;
	}

	assert (uchTerminalLink != USB_AUDIO_UNDEFINED_UNIT_ID);
	if (m_bIsOutput)
	{
		assert (m_nTerminals == 1);

		m_DeviceInfo.Terminal[0].TerminalType =
			pControlDevice->GetTerminalType (uchTerminalLink, FALSE);

		// get access to the Feature Unit, to control volume etc.
		m_uchFeatureUnitID[0] =
			pControlDevice->GetFeatureUnitID (uchTerminalLink, FALSE);
	}
	else
	{
		m_uchSelectorUnitID = pControlDevice->GetSelectorUnitID (uchTerminalLink);
		if (m_uchSelectorUnitID == USB_AUDIO_UNDEFINED_UNIT_ID)
		{
			assert (m_nTerminals == 1);

			m_DeviceInfo.Terminal[0].TerminalType =
				pControlDevice->GetTerminalType (uchTerminalLink, TRUE);

			m_uchFeatureUnitID[0] =
				pControlDevice->GetFeatureUnitID (uchTerminalLink, TRUE);
		}
		else
		{
			m_nTerminals = pControlDevice->GetNumSources (m_uchSelectorUnitID);
			assert (m_nTerminals);
			if (m_nTerminals > MaxTerminals)
			{
				m_nTerminals = MaxTerminals;
			}

			for (unsigned i = 0; i < m_nTerminals; i++)
			{
				u8 uchSourceID =
					pControlDevice->GetSourceID (m_uchSelectorUnitID, i);

				m_DeviceInfo.Terminal[i].TerminalType =
					pControlDevice->GetTerminalType (uchSourceID, TRUE);

				m_uchFeatureUnitID[i] =
					pControlDevice->GetFeatureUnitID (uchSourceID, TRUE);
			}
		}
	}

	m_DeviceInfo.NumTerminals = m_nTerminals;

	if (!InitTerminalControlInfo (pControlDevice))
	{
		return FALSE;
	}

	if (   !m_bIsOutput
	    && !SelectInputTerminal (0))
	{
		LOGWARN ("Cannot select default input terminal");

		return FALSE;
	}

	// prepare write supported sample rates info to log
	CString SampleRates;
	for (unsigned i = 0; i < m_DeviceInfo.SampleRateRanges; i++)
	{
		CString String;
		if (   m_DeviceInfo.SampleRateRange[i].Min
		    != m_DeviceInfo.SampleRateRange[i].Max)
		{
			// continuous subrange
			String.Format ("%u-%u/%u",
					m_DeviceInfo.SampleRateRange[i].Min,
					m_DeviceInfo.SampleRateRange[i].Max,
					m_DeviceInfo.SampleRateRange[i].Resolution);
		}
		else
		{
			// discrete rate
			String.Format ("%u", m_DeviceInfo.SampleRateRange[i].Min);
		}

		if (i)
		{
			SampleRates.Append (", ");
		}

		SampleRates.Append (String);
	}

	CString TerminalTypes;
	for (unsigned i = 0; i < m_nTerminals; i++)
	{
		CString String;
		String.Format ("0x%X", (unsigned) m_DeviceInfo.Terminal[i].TerminalType);

		if (i)
		{
			TerminalTypes.Append (", ");
		}

		TerminalTypes.Append (String);
	}

	m_DeviceName.Format (DeviceNamePattern,
			     pControlDevice->GetDeviceNumber (),
			     pControlDevice->GetNextStreamingSubDeviceNumber ());

	CDeviceNameService::Get ()->AddDevice (m_DeviceName, this, FALSE);

	From = m_DeviceName;	// for logger

	LOGNOTE ("%sput Terminal type(s): %s", m_bIsOutput ? "Out" : "In",
					       (const char *) TerminalTypes);
	LOGNOTE ("Supported sample rate(s): %s Hz", (const char *) SampleRates);

	return TRUE;
}

CUSBAudioStreamingDevice::TDeviceInfo CUSBAudioStreamingDevice::GetDeviceInfo (void) const
{
	return m_DeviceInfo;
}

boolean CUSBAudioStreamingDevice::Setup (unsigned nSampleRate)
{
	assert (m_pEndpointData);

	// is sample rate supported?
	unsigned i;
	for (i = 0; i < m_DeviceInfo.SampleRateRanges; i++)
	{
		if (   m_DeviceInfo.SampleRateRange[i].Min >= nSampleRate
		    && m_DeviceInfo.SampleRateRange[i].Max <= nSampleRate)
		{
			break;
		}
	}

	if (i >= m_DeviceInfo.SampleRateRanges)
	{
		LOGWARN ("Sample rate is not supported (%u)", nSampleRate);

		return FALSE;
	}

	DMA_BUFFER (u32, tSampleFreq, 1);
	tSampleFreq[0] = nSampleRate;
	if (!m_bVer200)
	{
		if (GetHost ()->ControlMessage (GetEndpoint0 (),
						REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_ENDPOINT,
						USB_AUDIO_REQ_SET_CUR,
						USB_AUDIO_CS_SAM_FREQ_CONTROL << 8,
						  m_pEndpointData->GetNumber ()
						| (m_pEndpointData->IsDirectionIn () ? 0x80 : 0),
						tSampleFreq, 3) < 0)
		{
			LOGDBG ("Cannot set sample rate");

			return FALSE;
		}
	}
	else
	{
		assert (m_uchClockSourceID != USB_AUDIO_UNDEFINED_UNIT_ID);
		if (GetHost ()->ControlMessage (GetEndpoint0 (),
						REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_INTERFACE,
						USB_AUDIO_REQ_SET_CUR,
						USB_AUDIO_CS_SAM_FREQ_CONTROL << 8,
						m_uchClockSourceID << 8,
						tSampleFreq, 4) < 0)
		{
			LOGDBG ("Cannot set sample rate");

			return FALSE;
		}
	}

	m_nSampleRate = nSampleRate;

	if (m_bSynchronousSync)
	{
		UpdateChunkSize ();
	}
	else
	{
		if (m_bIsOutput)
		{
			m_nChunkSizeBytes =   nSampleRate * m_nChannels * SUBFRAME_SIZE
					    / CHUNK_FREQUENCY;
		}
		else
		{
			m_nChunkSizeBytes = m_pEndpointData->GetMaxPacketSize ();
		}
	}

	return TRUE;
}

unsigned CUSBAudioStreamingDevice::GetChunkSizeBytes (void) const
{
	assert (m_nChunkSizeBytes);
	return m_nChunkSizeBytes;
}

boolean CUSBAudioStreamingDevice::SendChunk (const void *pBuffer, unsigned nChunkSizeBytes,
					     TCompletionRoutine *pCompletionRoutine, void *pParam)
{
	assert (pBuffer);

	assert (m_pEndpointData);
	CUSBRequest *pURB = new CUSBRequest (m_pEndpointData, (void *) pBuffer, nChunkSizeBytes);
	assert (pURB);

	if (m_bSynchronousSync)
	{
		assert (m_nPacketsPerChunk > 0);
		for (unsigned i = 0; i < m_nPacketsPerChunk; i++)
		{
			pURB->AddIsoPacket (m_usPacketSizeBytes[i]);
		}
	}
	else
	{
		pURB->AddIsoPacket (nChunkSizeBytes);
	}

	pURB->SetCompletionRoutine (CompletionHandler, pParam, (void *) pCompletionRoutine);

	boolean bOK = GetHost ()->SubmitAsyncRequest (pURB);

	if (   bOK
	    && m_pEndpointSync
	    && !m_bSyncEPActive)
	{
		m_bSyncEPActive = TRUE;

		u16 usPacketSize = GetDevice ()->GetSpeed () == USBSpeedFull ? 3 : 4;

		assert (m_pEndpointSync);
		CUSBRequest *pURBSync = new CUSBRequest (m_pEndpointSync, &m_SyncEPBuffer,
							 usPacketSize);
		assert (pURBSync);

		pURBSync->AddIsoPacket (usPacketSize);

		pURBSync->SetCompletionRoutine (SyncCompletionHandler, nullptr, this);

		bOK = GetHost ()->SubmitAsyncRequest (pURBSync);
	}
	else if (   bOK
		 && m_bSynchronousSync)
	{
		UpdateChunkSize ();
	}

	return bOK;
}

boolean CUSBAudioStreamingDevice::ReceiveChunk (void *pBuffer, unsigned nChunkSizeBytes,
					        TCompletionRoutine *pCompletionRoutine, void *pParam)
{
	assert (pBuffer);

	assert (m_pEndpointData);
	CUSBRequest *pURB = new CUSBRequest (m_pEndpointData, pBuffer, nChunkSizeBytes);
	assert (pURB);

	assert (!m_pEndpointSync);
	if (m_bSynchronousSync)
	{
		assert (m_nPacketsPerChunk > 0);
		for (unsigned i = 0; i < m_nPacketsPerChunk; i++)
		{
			pURB->AddIsoPacket (m_usPacketSizeBytes[i]);
		}
	}
	else
	{
		pURB->AddIsoPacket (nChunkSizeBytes);
	}

	pURB->SetCompletionRoutine (CompletionHandler, pParam, (void *) pCompletionRoutine);

	boolean bOK = GetHost ()->SubmitAsyncRequest (pURB);

	if (   bOK
	    && m_bSynchronousSync)
	{
		UpdateChunkSize ();
	}

	return bOK;
}

boolean CUSBAudioStreamingDevice::SelectInputTerminal (unsigned nIndex)
{
	assert (m_nTerminals);
	if (m_nTerminals == 1)
	{
		return TRUE;
	}

	assert (!m_bIsOutput);
	assert (nIndex < m_nTerminals);
	assert (m_uchSelectorUnitID != USB_AUDIO_UNDEFINED_UNIT_ID);

	DMA_BUFFER (u8, SelectorBuffer, 1);
	SelectorBuffer[0] = nIndex+1;

	if (GetHost ()->ControlMessage (GetEndpoint0 (),
					REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_INTERFACE,
					USB_AUDIO_REQ_SET_CUR,
					m_bVer200 ? USB_AUDIO_SU_SELECTOR_CONTROL << 8 | 0x00 : 0,
					m_uchSelectorUnitID << 8,
					SelectorBuffer, 1) < 0)
	{
		return FALSE;
	}

	m_nActiveTerminal = nIndex;

	return TRUE;
}

boolean CUSBAudioStreamingDevice::SetMute (boolean bEnable)
{
	assert (m_nActiveTerminal < m_nTerminals);

	if (!m_DeviceInfo.Terminal[m_nActiveTerminal].MuteSupported)
	{
		return FALSE;
	}

	assert (m_uchFeatureUnitID[m_nActiveTerminal] != USB_AUDIO_UNDEFINED_UNIT_ID);

	DMA_BUFFER (u8, MuteBuffer, 1);
	MuteBuffer[0] = bEnable ? 1 : 0;

	// same request for v1.00 and v2.00
	if (GetHost ()->ControlMessage (GetEndpoint0 (),
					REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_INTERFACE,
					USB_AUDIO_REQ_SET_CUR,
					USB_AUDIO_FU_MUTE_CONTROL << 8 | 0x00,	// master channel
					m_uchFeatureUnitID[m_nActiveTerminal] << 8,
					MuteBuffer, 1) < 0)
	{
		return FALSE;
	}

	return TRUE;
}

boolean CUSBAudioStreamingDevice::SetVolume (unsigned nChannel, int ndB)
{
	assert (nChannel <= 2);
	assert (m_nActiveTerminal < m_nTerminals);

	if (!m_DeviceInfo.Terminal[m_nActiveTerminal].VolumeSupported)
	{
		return FALSE;
	}

	if (   !m_DeviceInfo.Terminal[m_nActiveTerminal].VolumePerChannel
	    && nChannel)
	{
		return FALSE;
	}

	assert (m_uchFeatureUnitID[m_nActiveTerminal] != USB_AUDIO_UNDEFINED_UNIT_ID);

	DMA_BUFFER (s16, VolumeBuffer, 1);
	VolumeBuffer[0] = ndB << 8;

	// same request for v1.00 and v2.00
	if (GetHost ()->ControlMessage (GetEndpoint0 (),
					REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_INTERFACE,
					USB_AUDIO_REQ_SET_CUR,
					USB_AUDIO_FU_VOLUME_CONTROL << 8 | nChannel,
					m_uchFeatureUnitID[m_nActiveTerminal] << 8,
					VolumeBuffer, 2) < 0)
	{
		return FALSE;
	}

	return TRUE;
}

boolean CUSBAudioStreamingDevice::InitTerminalControlInfo (CUSBAudioControlDevice *pControlDevice)
{
	assert (pControlDevice);
	assert (m_nTerminals);

	for (unsigned i = 0; i < m_nTerminals; i++)
	{
		u8 uchFeatureUnitID = m_uchFeatureUnitID[i];
		if (uchFeatureUnitID == USB_AUDIO_UNDEFINED_UNIT_ID)
		{
			continue;
		}

		TDeviceInfo::TTerminalInfo *pInfo = &m_DeviceInfo.Terminal[i];

		pInfo->MuteSupported = pControlDevice->IsControlSupported (uchFeatureUnitID, 0,
								CUSBAudioFeatureUnit::MuteControl);

		u8 uchChannel = 0;		// master channel
		if (   pControlDevice->IsControlSupported (uchFeatureUnitID, 1,
							   CUSBAudioFeatureUnit::VolumeControl)
		    && pControlDevice->IsControlSupported (uchFeatureUnitID, 2,
							   CUSBAudioFeatureUnit::VolumeControl))
		{
			uchChannel = 1;		// left channel, should be same as right
		}
		else if (!pControlDevice->IsControlSupported (uchFeatureUnitID, 0,
							      CUSBAudioFeatureUnit::VolumeControl))
		{
			continue;
		}

		if (!m_bVer200)
		{
			DMA_BUFFER (s16, VolumeBuffer, 1);
			if (GetHost ()->ControlMessage (GetEndpoint0 (),
						REQUEST_IN | REQUEST_CLASS | REQUEST_TO_INTERFACE,
						USB_AUDIO_REQ_GET_MIN,
						USB_AUDIO_FU_VOLUME_CONTROL << 8 | uchChannel,
						uchFeatureUnitID << 8,
						VolumeBuffer, 2) < 0)
			{
				LOGWARN ("Cannot get volume minimum");

				return FALSE;
			}

			pInfo->MinVolume = VolumeBuffer[0] >> 8;

			if (GetHost ()->ControlMessage (GetEndpoint0 (),
						REQUEST_IN | REQUEST_CLASS | REQUEST_TO_INTERFACE,
						USB_AUDIO_REQ_GET_MAX,
						USB_AUDIO_FU_VOLUME_CONTROL << 8 | uchChannel,
						uchFeatureUnitID << 8,
						VolumeBuffer, 2) < 0)
			{
				LOGWARN ("Cannot get volume maximum");

				return FALSE;
			}

			pInfo->MaxVolume = VolumeBuffer[0] >> 8;
		}
		else
		{
			DMA_BUFFER (s16, VolumeBuffer, 4);
			if (GetHost ()->ControlMessage (GetEndpoint0 (),
						REQUEST_IN | REQUEST_CLASS | REQUEST_TO_INTERFACE,
						USB_AUDIO_REQ_RANGE,
						USB_AUDIO_FU_VOLUME_CONTROL << 8 | uchChannel,
						uchFeatureUnitID << 8,
						VolumeBuffer, 8) < 0)
			{
				LOGWARN ("Cannot get volume range");

				return FALSE;
			}

			if (VolumeBuffer[0] == 1)
			{
				pInfo->MinVolume = VolumeBuffer[1] >> 8;
				pInfo->MaxVolume = VolumeBuffer[2] >> 8;
			}
			else
			{
				continue;
			}
		}

		pInfo->VolumeSupported = TRUE;
		pInfo->VolumePerChannel = uchChannel != 0;
	}

	return TRUE;
}

void CUSBAudioStreamingDevice::CompletionHandler (CUSBRequest *pURB, void *pParam, void *pContext)
{
	assert (pURB);
	u32 nBytesTransferred = pURB->GetStatus () ? pURB->GetResultLength () : 0;
	delete pURB;

	TCompletionRoutine *pCompletionRoutine = (TCompletionRoutine *) pContext;
	if (pCompletionRoutine)
	{
		(*pCompletionRoutine) (nBytesTransferred, pParam);
	}
}

void CUSBAudioStreamingDevice::SyncCompletionHandler (CUSBRequest *pURB, void *pParam,
						      void *pContext)
{
	CUSBAudioStreamingDevice *pThis = (CUSBAudioStreamingDevice *) pContext;
	assert (pThis);

	assert (pThis->m_bIsOutput);

	assert (pURB);
	boolean bOK = !!pURB->GetStatus ();
	boolean bFormat10_14 = bOK && pURB->GetResultLength () == 3;

	delete pURB;

	assert (pThis->m_bSyncEPActive);

	if (bOK)
	{
		if (bFormat10_14)
		{
			// Q10.14 format (FS)
			pThis->m_nSyncAccu += pThis->m_SyncEPBuffer[0] & 0xFFFFFF;
			pThis->m_nChunkSizeBytes =   (pThis->m_nSyncAccu >> 14)
						   * CHANNELS * SUBFRAME_SIZE;
			pThis->m_nSyncAccu &= 0x3FFF;
		}
		else
		{
			// Q16.16 format (HS)
			pThis->m_nSyncAccu += pThis->m_SyncEPBuffer[0];
			pThis->m_nChunkSizeBytes =   (pThis->m_nSyncAccu >> 16)
						   * CHANNELS * SUBFRAME_SIZE;
			pThis->m_nSyncAccu &= 0xFFFF;
		}
	}

	pThis->m_bSyncEPActive = FALSE;
}

void CUSBAudioStreamingDevice::UpdateChunkSize (void)
{
	assert (m_bSynchronousSync);
	assert (m_nSampleRate > 0);

	unsigned nUSBFrameRate = GetDevice ()->GetSpeed () == USBSpeedFull ? 1000 : 8000;

	m_SpinLock.Acquire ();

	m_nPacketsPerChunk = nUSBFrameRate / CHUNK_FREQUENCY;

	unsigned nChunkSizeBytes = 0;
	for (unsigned i = 0; i < m_nPacketsPerChunk; i++)
	{
		m_nSyncAccu += m_nSampleRate;
		unsigned nFrames = m_nSyncAccu / nUSBFrameRate;
		m_nSyncAccu %= nUSBFrameRate;

		m_usPacketSizeBytes[i] = nFrames * m_nChannels * SUBFRAME_SIZE;

		nChunkSizeBytes += m_usPacketSizeBytes[i];
	}

	m_nChunkSizeBytes = nChunkSizeBytes;

	m_SpinLock.Release ();
}
