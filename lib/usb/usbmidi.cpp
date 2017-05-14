//
// usbmidi.c
//
// Ported from the USPi driver which is:
// 	Copyright (C) 2016  J. Otto <joshua.t.otto@gmail.com>
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

// Refer to "Universal Serial Bus Device Class Specification for MIDI Devices"

#include <circle/usb/usbmidi.h>
#include <circle/usb/usbaudio.h>
#include <circle/usb/usb.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/devicenameservice.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <assert.h>

#define EVENT_PACKET_SIZE	4

static const char FromMIDI[] = "umidi";

unsigned CUSBMIDIDevice::s_nDeviceNumber = 1;

// This handy table from the Linux driver encodes the mapping between MIDI
// packet Code Index Number values and encapsulated packet lengths.
static const unsigned cin_to_length[] = {
	0, 0, 2, 3, 3, 1, 2, 3, 3, 3, 3, 3, 2, 2, 3, 1
};

CUSBMIDIDevice::CUSBMIDIDevice (CUSBFunction *pFunction)
:	CUSBFunction (pFunction),
	m_pEndpointIn (0),
	m_pPacketHandler (0),
	m_pURB (0),
	m_pPacketBuffer (0),
	m_hTimer (0)
{
}

CUSBMIDIDevice::~CUSBMIDIDevice (void)
{
	if (m_pPacketBuffer != 0)
	{
		delete [] m_pPacketBuffer;
		m_pPacketBuffer = 0;
	}

	if (m_pEndpointIn != 0)
	{
		delete m_pEndpointIn;
		m_pEndpointIn = 0;
	}
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
		if (   (pEndpointDesc->bEndpointAddress & 0x80) != 0x80		// Input EP
		    || (pEndpointDesc->bmAttributes     & 0x3F) != 0x02)	// Bulk EP
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

	CString DeviceName;
	DeviceName.Format ("umidi%u", s_nDeviceNumber++);
	CDeviceNameService::Get ()->AddDevice (DeviceName, this, FALSE);

	return StartRequest ();
}

void CUSBMIDIDevice::RegisterPacketHandler (TMIDIPacketHandler *pPacketHandler)
{
	assert (m_pPacketHandler == 0);
	m_pPacketHandler = pPacketHandler;
	assert (m_pPacketHandler != 0);
}

boolean CUSBMIDIDevice::StartRequest (void)
{
	assert (m_pEndpointIn != 0);
	assert (m_pPacketBuffer != 0);

	assert (m_pURB == 0);
	assert (m_usBufferSize > 0);
	m_pURB = new CUSBRequest (m_pEndpointIn, m_pPacketBuffer, m_usBufferSize);
	assert (m_pURB != 0);
	m_pURB->SetCompletionRoutine (CompletionStub, 0, this);

	m_pURB->SetCompleteOnNAK ();	// do not retry if request cannot be served immediately

	return GetHost ()->SubmitAsyncRequest (m_pURB);
}

void CUSBMIDIDevice::CompletionRoutine (CUSBRequest *pURB)
{
	assert (pURB != 0);
	assert (m_pURB == pURB);

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

	delete m_pURB;
	m_pURB = 0;

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

void CUSBMIDIDevice::TimerHandler (unsigned hTimer)
{
	assert (m_hTimer == hTimer);
	m_hTimer = 0;

	StartRequest ();
}

void CUSBMIDIDevice::TimerStub (unsigned hTimer, void *pParam, void *pContext)
{
	CUSBMIDIDevice *pThis = (CUSBMIDIDevice *) pContext;
	assert (pThis != 0);

	pThis->TimerHandler (hTimer);
}
