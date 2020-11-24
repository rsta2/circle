//
// usbmidi.c
//
// Ported from the USPi driver which is:
// 	Copyright (C) 2016  J. Otto <joshua.t.otto@gmail.com>
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2020  R. Stange <rsta2@o2online.de>
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

// Refer to "Universal Serial Bus Device Class Specification for MIDI Devices"

#include <circle/usb/usbmidi.h>
#include <circle/usb/usbaudio.h>
#include <circle/usb/usb.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/devicenameservice.h>
#include <circle/synchronize.h>
#include <circle/logger.h>
#include <circle/debug.h>
#include <circle/util.h>
#include <assert.h>

#define EVENT_PACKET_SIZE	4

static const char FromMIDI[] = "umidi";
static const char DevicePrefix[] = "umidi";

CNumberPool CUSBMIDIDevice::s_DeviceNumberPool (1);

// This handy table from the Linux driver encodes the mapping between MIDI
// packet Code Index Number values and encapsulated packet lengths.
static const unsigned cin_to_length[] = {
	0, 0, 2, 3, 3, 1, 2, 3, 3, 3, 3, 3, 2, 2, 3, 1
};

CUSBMIDIDevice::CUSBMIDIDevice (CUSBFunction *pFunction)
:	CUSBFunction (pFunction),
	m_pEndpointIn (0),
	m_pEndpointOut (0),
	m_pPacketHandler (0),
	m_pPacketBuffer (0),
	m_hTimer (0),
	m_nDeviceNumber (0)
{
}

CUSBMIDIDevice::~CUSBMIDIDevice (void)
{
	if (m_hTimer != 0)
	{
		CTimer::Get ()->CancelKernelTimer (m_hTimer);
		m_hTimer = 0;
	}

	if (m_nDeviceNumber != 0)
	{
		CDeviceNameService::Get ()->RemoveDevice (DevicePrefix, m_nDeviceNumber, FALSE);

		s_DeviceNumberPool.FreeNumber (m_nDeviceNumber);
	}

	delete [] m_pPacketBuffer;
	m_pPacketBuffer = 0;

	delete m_pEndpointIn;
	m_pEndpointIn = 0;

	delete m_pEndpointOut;
	m_pEndpointOut = 0;
}

boolean CUSBMIDIDevice::Configure (void)
{
	if (GetNumEndpoints () < 1)
	{
		ConfigurationError (FromMIDI);

		return FALSE;
	}

	// special handling for Roland UM-ONE MIDI interface
	const TUSBDeviceDescriptor *pDeviceDesc = GetDevice ()->GetDeviceDescriptor ();
	assert (pDeviceDesc != 0);
	boolean bIsRolandUMOne =    pDeviceDesc->idVendor  == 0x0582
				 && pDeviceDesc->idProduct == 0x012A;

	// Our strategy for now is simple: we'll take the first MIDI streaming
	// bulk-in endpoint on this interface we can find.  To distinguish
	// between the MIDI streaming bulk-in endpoints we want (which carry
	// actual MIDI data streams) and 'transfer bulk data' endpoints (which
	// are used to implement features like Downloadable Samples that we
	// don't care about), we'll look for an immediately-accompanying
	// class-specific endpoint descriptor.
	TUSBAudioEndpointDescriptor *pEndpointDesc;
	while ((pEndpointDesc = (TUSBAudioEndpointDescriptor *) GetDescriptor (DESCRIPTOR_ENDPOINT)) != 0)
	{
		if ((pEndpointDesc->bmAttributes & 0x3F) != 0x02)	// Bulk EP
		{
			continue;
		}

		if (!bIsRolandUMOne)
		{
			TUSBMIDIStreamingEndpointDescriptor *pMIDIDesc =
				(TUSBMIDIStreamingEndpointDescriptor *) GetDescriptor (DESCRIPTOR_CS_ENDPOINT);
			if (   pMIDIDesc == 0
			    || (u8 *) pEndpointDesc + pEndpointDesc->bLength != (u8 *) pMIDIDesc)
			{
				continue;
			}
		}

		if ((pEndpointDesc->bEndpointAddress & 0x80) == 0x80)	// Input EP
		{
			if (m_pEndpointIn != 0)
			{
				ConfigurationError (FromMIDI);

				return FALSE;
			}

			m_pEndpointIn = new CUSBEndpoint (GetDevice (), (TUSBEndpointDescriptor *) pEndpointDesc);
			assert (m_pEndpointIn != 0);

			m_usBufferSize  = pEndpointDesc->wMaxPacketSize;
			m_usBufferSize -= pEndpointDesc->wMaxPacketSize % EVENT_PACKET_SIZE;

			assert (m_pPacketBuffer == 0);
			m_pPacketBuffer = new u8[m_usBufferSize];
			assert (m_pPacketBuffer != 0);
		}
		else							// Output EP
		{
			if (m_pEndpointOut != 0)
			{
				ConfigurationError (FromMIDI);

				return FALSE;
			}

			m_pEndpointOut = new CUSBEndpoint (GetDevice (), (TUSBEndpointDescriptor *) pEndpointDesc);
			assert (m_pEndpointOut != 0);
		}

	}

	if (m_pEndpointIn == 0)
	{
		ConfigurationError (FromMIDI);

		return FALSE;
	}

	if (!CUSBFunction::Configure ())
	{
		CLogger::Get ()->Write (FromMIDI, LogError, "Cannot set interface");

		return FALSE;
	}

	assert (m_nDeviceNumber == 0);
	m_nDeviceNumber = s_DeviceNumberPool.AllocateNumber (TRUE, FromMIDI);

	CDeviceNameService::Get ()->AddDevice (DevicePrefix, m_nDeviceNumber, this, FALSE);

	return StartRequest ();
}

void CUSBMIDIDevice::RegisterPacketHandler (TMIDIPacketHandler *pPacketHandler)
{
	assert (m_pPacketHandler == 0);
	m_pPacketHandler = pPacketHandler;
	assert (m_pPacketHandler != 0);
}

boolean CUSBMIDIDevice::SendEventPackets (const u8 *pData, unsigned nLength)
{
	assert (pData != 0);
	assert (nLength > 0);
	assert ((nLength & 3) == 0);

	if (m_pEndpointOut == 0)
	{
		return FALSE;
	}

	DMA_BUFFER (u8, Buffer, nLength);
	memcpy (Buffer, pData, nLength);

	return GetHost ()->Transfer (m_pEndpointOut, Buffer, nLength) == (int) nLength;
}

boolean CUSBMIDIDevice::SendPlainMIDI (unsigned nCable, const u8 *pData, unsigned nLength)
{
	assert (nCable <= 15);
	assert (pData != 0);
	assert (nLength > 0);

	if (m_pEndpointOut == 0)
	{
		return FALSE;
	}

	size_t nBufferSize = nLength * 4;		// worst case
	size_t nBufferValid = 0;
	DMA_BUFFER (u8, Buffer, nBufferSize);
	u8 *pBuffer = Buffer;

	unsigned nState = 0;
	unsigned nPacketLength;
	u8 *pSysExStart;
	while (nLength--)
	{
		u8 uchByte = *pData++;
		switch (nState)
		{
		case 0:					// handle MIDI status code
			if (uchByte < 0xF0)		// channel voice message
			{
				*pBuffer++ = (nCable << 4) | (uchByte >> 4);
				nBufferValid++;
				nPacketLength = cin_to_length[uchByte >> 4];
				nState = 1;
			}
			else				// system common messages
			{
				switch (uchByte)
				{
				case 0xF0:		// sysex
					pSysExStart = pBuffer;
					*pBuffer++ = (nCable << 4) | 4;
					nBufferValid++;
					nPacketLength = 0;
					nState = 2;
					goto StateSysEx;

				case 0xF1:		// time code
				case 0xF3:		// song select
					*pBuffer++ = (nCable << 4) | 2;
					nBufferValid++;
					nPacketLength = 2;
					nState = 1;
					break;

				case 0xF2:		// song position pointer
					*pBuffer++ = (nCable << 4) | 3;
					nBufferValid++;
					nPacketLength = 3;
					nState = 1;
					break;

				case 0xF6:		// tune request
				case 0xF8:		// timing clock
				case 0xFA:		// start
				case 0xFB:		// continue
				case 0xFC:		// stop
				case 0xFE:		// active sensing
				case 0xFF:		// reset
					*pBuffer++ = (nCable << 4) | 5;
					nBufferValid++;
					nPacketLength = 1;
					nState = 1;
					break;

				default:
					CLogger::Get ()->Write (FromMIDI, LogWarning,
								"Unsupported MIDI status code (0x%X)",
								(unsigned) uchByte);
					return FALSE;
				}
			}
			// fall through

		case 1:					// copy MIDI data bytes
			*pBuffer++ = uchByte;
			nBufferValid++;
			if (--nPacketLength == 0)
			{
				while ((nBufferValid & 3) != 0)		// padding
				{
					*pBuffer++ = 0;
					nBufferValid++;
				}

				nState = 0;
			}
			break;

		StateSysEx:				// handle sysex message
		case 2:
			if (nPacketLength == 3)		// current event packet full?
			{
				pSysExStart = pBuffer;	// start next packet
				*pBuffer++ = (nCable << 4) | 4;
				nBufferValid++;
				nPacketLength = 0;
			}

			*pBuffer++ = uchByte;		// copy sysex data bytes
			nBufferValid++;
			nPacketLength++;

			if (uchByte == 0xF7)		// end of sysex message?
			{
				*pSysExStart = (nCable << 4) | (nPacketLength + 4);	// set CIN

				while (nPacketLength++ < 3)	// padding
				{
					*pBuffer++ = 0;
					nBufferValid++;
				}

				nState = 0;
			}
			break;

		default:
			assert (0);
			break;
		}
	}

	if (nState != 0)
	{
		CLogger::Get ()->Write (FromMIDI, LogWarning, "Incomplete MIDI message");

		return FALSE;
	}

	//debug_hexdump (Buffer, nBufferValid, FromMIDI);

	return GetHost ()->Transfer (m_pEndpointOut, Buffer, nBufferValid) == (int) nBufferValid;
}

boolean CUSBMIDIDevice::StartRequest (void)
{
	assert (m_pEndpointIn != 0);
	assert (m_pPacketBuffer != 0);

	assert (m_usBufferSize > 0);
	CUSBRequest *pURB = new CUSBRequest (m_pEndpointIn, m_pPacketBuffer, m_usBufferSize);
	assert (pURB != 0);
	pURB->SetCompletionRoutine (CompletionStub, 0, this);

	pURB->SetCompleteOnNAK ();	// do not retry if request cannot be served immediately

	return GetHost ()->SubmitAsyncRequest (pURB);
}

void CUSBMIDIDevice::CompletionRoutine (CUSBRequest *pURB)
{
	assert (pURB != 0);

	boolean bRestart = FALSE;

	if (   pURB->GetStatus () != 0
	    && pURB->GetResultLength () % EVENT_PACKET_SIZE == 0)
	{
		assert (m_pPacketBuffer != 0);

		u8 *pEnd = m_pPacketBuffer + pURB->GetResultLength ();
		for (u8 *pPacket = m_pPacketBuffer; pPacket < pEnd; pPacket += EVENT_PACKET_SIZE)
		{
			// Follow the Linux driver's example and ignore packets with Cable
			// Number == Code Index Number == 0, which some devices seem to
			// generate as padding in spite of their status as reserved.
			if (pPacket[0] != 0)
			{
				if (m_pPacketHandler != 0)
				{
					unsigned nCable = pPacket[0] >> 4;
					unsigned nLength = cin_to_length[pPacket[0] & 0x0F];
					(*m_pPacketHandler) (nCable, pPacket+1, nLength);
				}

				bRestart = TRUE;
			}
		}
	}

	delete pURB;

	if (bRestart)
	{
		StartRequest ();
	}
	else
	{
		assert (m_hTimer == 0);
		m_hTimer = CTimer::Get ()->StartKernelTimer (MSEC2HZ (10), TimerStub, 0, this);
		assert (m_hTimer != 0);
	}
}

void CUSBMIDIDevice::CompletionStub (CUSBRequest *pURB, void *pParam, void *pContext)
{
	CUSBMIDIDevice *pThis = (CUSBMIDIDevice *) pContext;
	assert (pThis != 0);

	pThis->CompletionRoutine (pURB);
}

void CUSBMIDIDevice::TimerHandler (TKernelTimerHandle hTimer)
{
	assert (m_hTimer == hTimer);
	m_hTimer = 0;

	StartRequest ();
}

void CUSBMIDIDevice::TimerStub (TKernelTimerHandle hTimer, void *pParam, void *pContext)
{
	CUSBMIDIDevice *pThis = (CUSBMIDIDevice *) pContext;
	assert (pThis != 0);

	pThis->TimerHandler (hTimer);
}
