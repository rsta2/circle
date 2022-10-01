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
#include <circle/usb/usbaudio.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/devicenameservice.h>
#include <circle/synchronize.h>
#include <circle/logger.h>
#include <circle/string.h>
#include <circle/debug.h>
#include <circle/util.h>
#include <assert.h>

// supported format
#define CHANNELS		2		// Stereo
#define SUBFRAME_SIZE		2		// 16-bit signed
#define CHUNK_FREQUENCY		1000		// per second

// Audio class control request
#define SET_CUR			0x01
#define SAMPLING_FREQ_CONTROL	0x01

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
	m_nChunkSizeBytes (0),
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

	delete m_pEndpointOut;
	m_pEndpointOut = nullptr;
}

boolean CUSBAudioStreamingDevice::Initialize (void)
{
	if (!CUSBFunction::Initialize ())
	{
		return FALSE;
	}

	return GetNumEndpoints () == 1;		// ignore no-endpoint interfaces
}

boolean CUSBAudioStreamingDevice::Configure (void)
{
	assert (GetNumEndpoints () == 1);

	TUSBAudioTypeIFormatTypeDescriptor *pFormatTypeDesc;
	while ((pFormatTypeDesc = (TUSBAudioTypeIFormatTypeDescriptor *)
					GetDescriptor (DESCRIPTOR_CS_INTERFACE)) != nullptr)
	{
		if (pFormatTypeDesc->bDescriptorSubtype == 0x02)	// FORMAT_TYPE
		{
			break;
		}
	}

	if (!pFormatTypeDesc)
	{
		LOGWARN ("FORMAT_TYPE descriptor expected");

		return FALSE;
	}

	TUSBAudioEndpointDescriptor *pEndpointDesc;
	pEndpointDesc = (TUSBAudioEndpointDescriptor *) GetDescriptor (DESCRIPTOR_ENDPOINT);
	if (   !pEndpointDesc
	    || (pEndpointDesc->bmAttributes & 0x3F)     != 0x09	 // Isochronous, Adaptive, Data
	    || (pEndpointDesc->bEndpointAddress & 0x80) != 0x00) // Output EP
	{
		return FALSE;
	}

	if (   GetDevice ()->GetSpeed () != USBSpeedFull		// TODO
	    || pEndpointDesc->bInterval  != 1)
	{
		LOGWARN ("Unsupported EP timing");

		return FALSE;
	}

	if (   pFormatTypeDesc->bFormatType    != 0x01			// FORMAT_TYPE_I
	    || pFormatTypeDesc->bNrChannels    != CHANNELS
	    || pFormatTypeDesc->bSubframeSize  != SUBFRAME_SIZE
	    || pFormatTypeDesc->bBitResolution != SUBFRAME_SIZE*8)
	{
		LOGWARN ("Invalid output format");
#ifndef NDEBUG
		debug_hexdump (pFormatTypeDesc, sizeof *pFormatTypeDesc, From);
#endif

		return FALSE;
	}

	// fetch format info from descriptor
	if (pFormatTypeDesc->bSamFreqType == 0)
	{
		m_FormatInfo.SampleRateType = SampleRateTypeContinous;
		m_FormatInfo.LowerSampleRate = RATE2UNSIGNED (pFormatTypeDesc->tSamFreq[0]);
		m_FormatInfo.UpperSampleRate = RATE2UNSIGNED (pFormatTypeDesc->tSamFreq[1]);

		LOGDBG ("Continous sample rate (%u-%u Hz)",
			m_FormatInfo.LowerSampleRate, m_FormatInfo.UpperSampleRate);
	}
	else
	{
		unsigned nSampleRates = pFormatTypeDesc->bSamFreqType;
		if (nSampleRates > MaxDiscreteSampleRates)
		{
			nSampleRates = MaxDiscreteSampleRates;
		}
		m_FormatInfo.SampleRateType = nSampleRates;

		CString SampleRates;

		for (unsigned i = 0; i < nSampleRates; i++)
		{
			m_FormatInfo.DiscreteSampleRate[i] =
				RATE2UNSIGNED (pFormatTypeDesc->tSamFreq[i]);

			CString String;
			String.Format ("%u", m_FormatInfo.DiscreteSampleRate[i]);

			if (i)
			{
				SampleRates.Append (", ");
			}

			SampleRates.Append (String);
		}

		LOGDBG ("Discrete sample rate(s): %s Hz", (const char *) SampleRates);
	}

	m_pEndpointOut = new CUSBEndpoint (GetDevice (), (TUSBEndpointDescriptor *) pEndpointDesc);
	assert (m_pEndpointOut != 0);

	if (!CUSBFunction::Configure ())
	{
		LOGWARN ("Cannot set interface");

		return FALSE;
	}

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
	if (m_FormatInfo.SampleRateType == SampleRateTypeContinous)
	{
		if (   nSampleRate < m_FormatInfo.LowerSampleRate
		    || nSampleRate > m_FormatInfo.UpperSampleRate)
		{
			LOGWARN ("Sample rate out of range (%u)", nSampleRate);

			return FALSE;
		}
	}
	else
	{
		unsigned i;
		for (i = 0; i < m_FormatInfo.SampleRateType; i++)
		{
			if (m_FormatInfo.DiscreteSampleRate[i] == nSampleRate)
			{
				break;
			}
		}

		if (i >= m_FormatInfo.SampleRateType)
		{
			LOGWARN ("Sample rate is not supported (%u)", nSampleRate);

			return FALSE;
		}
	}

	DMA_BUFFER (u32, tSampleFreq, 1);
	tSampleFreq[0] = nSampleRate;

	if (GetHost ()->ControlMessage (GetEndpoint0 (),
					REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_ENDPOINT,
					SET_CUR, SAMPLING_FREQ_CONTROL << 8,
					m_pEndpointOut->GetNumber (),
					tSampleFreq, 3) < 0)
	{
		LOGDBG ("Cannot set sample rate");

		return FALSE;
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
	assert (nChunkSizeBytes <= m_nChunkSizeBytes);
	assert (nChunkSizeBytes <= m_pEndpointOut->GetMaxPacketSize ());	// TODO

	assert (m_pEndpointOut);
	CUSBRequest *pURB = new CUSBRequest (m_pEndpointOut, (void *) pBuffer, nChunkSizeBytes);
	assert (pURB);

	pURB->SetCompletionRoutine (CompletionStub, pParam, (void *) pCompletionRoutine);

	return GetHost ()->SubmitAsyncRequest (pURB);
}

void CUSBAudioStreamingDevice::CompletionStub (CUSBRequest *pURB, void *pParam, void *pContext)
{
	assert (pURB);
	delete pURB;

	TCompletionRoutine *pCompletionRoutine = (TCompletionRoutine *) (pContext);
	if (pCompletionRoutine)
	{
		(*pCompletionRoutine) (pParam);
	}
}
