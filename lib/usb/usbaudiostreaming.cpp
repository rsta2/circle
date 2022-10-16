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
#include <circle/usb/usbaudiostreaming.h>
#include <circle/usb/usbaudiocontrol.h>
#include <circle/usb/usbaudio.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/devicenameservice.h>
#include <circle/logger.h>
#include <circle/string.h>
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

LOGMODULE ("uaudio");
static const char DevicePrefix[] = "uaudio";

CNumberPool CUSBAudioStreamingDevice::s_DeviceNumberPool (1);

CUSBAudioStreamingDevice::CUSBAudioStreamingDevice (CUSBFunction *pFunction)
:	CUSBFunction (pFunction),
	m_pEndpointOut (nullptr),
	m_pEndpointSync (nullptr),
	m_nChunkSizeBytes (0),
	m_bSyncEPActive (FALSE),
	m_nSyncAccu (0),
	m_uchClockSourceID (USB_AUDIO_UNDEFINED_UNIT_ID),
	m_pCompletionRoutine (nullptr),
	m_nDeviceNumber (0)
{
	memset (&m_FormatInfo, 0, sizeof m_FormatInfo);
}

CUSBAudioStreamingDevice::~CUSBAudioStreamingDevice (void)
{
	if (m_nDeviceNumber)
	{
		CDeviceNameService::Get ()->RemoveDevice (DevicePrefix, m_nDeviceNumber, FALSE);

		s_DeviceNumberPool.FreeNumber (m_nDeviceNumber);
	}

	delete m_pEndpointSync;
	m_pEndpointSync = nullptr;

	delete m_pEndpointOut;
	m_pEndpointOut = nullptr;
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
	    || (pEndpointDesc->bmAttributes     & 0x33) != 0x01	 // Isochronous, Data
	    || (pEndpointDesc->bEndpointAddress & 0x80) != 0x00) // Output EP
	{
		LOGWARN ("Isochronous data output EP expected");

		return FALSE;
	}

	if (   GetDevice ()->GetSpeed () != USBSpeedFull		// TODO: HS
	    || pEndpointDesc->bInterval  != 1)
	{
		LOGWARN ("Unsupported EP timing");

		return FALSE;
	}

	if (!m_bVer200)
	{
		if (   pFormatTypeDesc->bFormatType           != USB_AUDIO_FORMAT_TYPE_I
		    || pFormatTypeDesc->Ver100.bNrChannels    != CHANNELS
		    || pFormatTypeDesc->Ver100.bSubframeSize  != SUBFRAME_SIZE
		    || pFormatTypeDesc->Ver100.bBitResolution != SUBFRAME_SIZE*8)
		{
			LOGWARN ("Invalid output format");
#ifndef NDEBUG
			debug_hexdump (pFormatTypeDesc, pFormatTypeDesc->bLength, From);
#endif

			return FALSE;
		}
	}
	else
	{
		if (   pFormatTypeDesc->bFormatType           != USB_AUDIO_FORMAT_TYPE_I
		    || pFormatTypeDesc->Ver200.bSubslotSize   != SUBFRAME_SIZE
		    || pFormatTypeDesc->Ver200.bBitResolution != SUBFRAME_SIZE*8
		    || pGeneralDesc->Ver200.bNrChannels       != CHANNELS)
		{
			LOGWARN ("Invalid output format (chans %u)",
				 (unsigned) pGeneralDesc->Ver200.bNrChannels);
#ifndef NDEBUG
			debug_hexdump (pFormatTypeDesc, pFormatTypeDesc->bLength, From);
#endif

			return FALSE;
		}
	}

	if ((pEndpointDesc->bmAttributes & 0x0C) == 0x04)		// Asynchronous sync
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
	else if ((pEndpointDesc->bmAttributes & 0x0C) != 0x08)		// Adaptive sync
	{
		LOGWARN ("Unsupported EP synchronization");

		return FALSE;
	}

	m_pEndpointOut = new CUSBEndpoint (GetDevice (), (TUSBEndpointDescriptor *) pEndpointDesc);
	assert (m_pEndpointOut != 0);

	if (!CUSBFunction::Configure ())
	{
		LOGWARN ("Cannot set interface");

		return FALSE;
	}

	if (!m_bVer200)
	{
		// fetch format info from descriptor
		if (pFormatTypeDesc->Ver100.bSamFreqType == 0)
		{
			// continuous range
			m_FormatInfo.SampleRateRanges = 1;
			m_FormatInfo.SampleRateRange[0].Min =
				RATE2UNSIGNED (pFormatTypeDesc->Ver100.tSamFreq[0]);
			m_FormatInfo.SampleRateRange[0].Max =
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
			m_FormatInfo.SampleRateRanges = nSampleRates;

			for (unsigned i = 0; i < nSampleRates; i++)
			{
				m_FormatInfo.SampleRateRange[i].Min =
				m_FormatInfo.SampleRateRange[i].Max =
					RATE2UNSIGNED (pFormatTypeDesc->Ver100.tSamFreq[i]);
			}
		}
	}
	else
	{
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

		// request clock source ID for this Input Terminal
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

		// fill in the m_FormatInfo struct
		if (nSampleRates > MaxSampleRatesRanges)
		{
			nSampleRates = MaxSampleRatesRanges;
		}
		m_FormatInfo.SampleRateRanges = nSampleRates;

		u32 *pFreq = (u32 *) (RangesBuffer+2);
		for (unsigned i = 0; i < nSampleRates; i++)
		{
			m_FormatInfo.SampleRateRange[i].Min = *pFreq++;
			m_FormatInfo.SampleRateRange[i].Max = *pFreq++;
			m_FormatInfo.SampleRateRange[i].Resolution = *pFreq++;
		}
	}

	// write supported sample rates info to log
	CString SampleRates;
	for (unsigned i = 0; i < m_FormatInfo.SampleRateRanges; i++)
	{
		CString String;
		if (   m_FormatInfo.SampleRateRange[i].Min
		    != m_FormatInfo.SampleRateRange[i].Max)
		{
			// continuous subrange
			String.Format ("%u-%u/%u",
					m_FormatInfo.SampleRateRange[i].Min,
					m_FormatInfo.SampleRateRange[i].Max,
					m_FormatInfo.SampleRateRange[i].Resolution);
		}
		else
		{
			// discrete rate
			String.Format ("%u", m_FormatInfo.SampleRateRange[i].Min);
		}

		if (i)
		{
			SampleRates.Append (", ");
		}

		SampleRates.Append (String);
	}

	LOGNOTE ("Supported sample rate(s): %s Hz", (const char *) SampleRates);

	assert (m_nDeviceNumber == 0);
	m_nDeviceNumber = s_DeviceNumberPool.AllocateNumber (TRUE, From);

	CDeviceNameService::Get ()->AddDevice (DevicePrefix, m_nDeviceNumber, this, FALSE);

	return TRUE;
}

CUSBAudioStreamingDevice::TFormatInfo CUSBAudioStreamingDevice::GetFormatInfo (void) const
{
	return m_FormatInfo;
}

boolean CUSBAudioStreamingDevice::Setup (unsigned nSampleRate)
{
	// is sample rate supported?
	unsigned i;
	for (i = 0; i < m_FormatInfo.SampleRateRanges; i++)
	{
		if (   m_FormatInfo.SampleRateRange[i].Min >= nSampleRate
		    && m_FormatInfo.SampleRateRange[i].Max <= nSampleRate)
		{
			break;
		}
	}

	if (i >= m_FormatInfo.SampleRateRanges)
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
						m_pEndpointOut->GetNumber (),
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

	m_nChunkSizeBytes = nSampleRate * CHANNELS * SUBFRAME_SIZE / CHUNK_FREQUENCY;

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

	assert (m_pEndpointOut);
	CUSBRequest *pURB = new CUSBRequest (m_pEndpointOut, (void *) pBuffer, nChunkSizeBytes);
	assert (pURB);

	assert (!m_pCompletionRoutine);
	m_pCompletionRoutine = pCompletionRoutine;
	m_pCompletionParam = pParam;

	pURB->SetCompletionRoutine (CompletionHandler, nullptr, this);

	boolean bOK = GetHost ()->SubmitAsyncRequest (pURB);

	if (   bOK
	    && m_pEndpointSync
	    && !m_bSyncEPActive)
	{
		m_bSyncEPActive = TRUE;

		assert (m_pEndpointSync);
		CUSBRequest *pURBSync = new CUSBRequest (m_pEndpointSync, &m_SyncEPBuffer,
						GetDevice ()->GetSpeed () == USBSpeedFull ? 3 : 4);
		assert (pURBSync);

		pURBSync->SetCompletionRoutine (SyncCompletionHandler, nullptr, this);

		bOK = GetHost ()->SubmitAsyncRequest (pURBSync);
	}

	return bOK;
}

void CUSBAudioStreamingDevice::CompletionHandler (CUSBRequest *pURB, void *pParam, void *pContext)
{
	CUSBAudioStreamingDevice *pThis = (CUSBAudioStreamingDevice *) pContext;
	assert (pThis);

	assert (pURB);
	delete pURB;

	TCompletionRoutine *pCompletionRoutine = pThis->m_pCompletionRoutine;
	pThis->m_pCompletionRoutine = nullptr;

	if (pCompletionRoutine)
	{
		(*pCompletionRoutine) (pThis->m_pCompletionParam);
	}
}

void CUSBAudioStreamingDevice::SyncCompletionHandler (CUSBRequest *pURB, void *pParam,
						      void *pContext)
{
	CUSBAudioStreamingDevice *pThis = (CUSBAudioStreamingDevice *) pContext;
	assert (pThis);

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
