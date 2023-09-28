//
// usbmidihost.c
//
// Ported from the USPi driver which is:
// 	Copyright (C) 2016  J. Otto <joshua.t.otto@gmail.com>
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

// Refer to "Universal Serial Bus Device Class Specification for MIDI Devices"

#include <circle/usb/usbmidihost.h>
#include <circle/usb/usbaudio.h>
#include <circle/usb/usb.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/synchronize.h>
#include <circle/koptions.h>
#include <circle/logger.h>
#include <circle/debug.h>
#include <circle/util.h>
#include <assert.h>

static const char FromMIDI[] = "umidihost";

CUSBMIDIHostDevice::CUSBMIDIHostDevice (CUSBFunction *pFunction)
:	CUSBFunction (pFunction),
	m_pInterface (0),
	m_pEndpointIn (0),
	m_pEndpointOut (0),
	m_pPacketBuffer (0),
	m_hTimer (0)
{
}

CUSBMIDIHostDevice::~CUSBMIDIHostDevice (void)
{
	if (m_hTimer != 0)
	{
		CTimer::Get ()->CancelKernelTimer (m_hTimer);
		m_hTimer = 0;
	}

	delete m_pInterface;
	m_pInterface = 0;

	delete [] m_pPacketBuffer;
	m_pPacketBuffer = 0;

	delete m_pEndpointIn;
	m_pEndpointIn = 0;

	delete m_pEndpointOut;
	m_pEndpointOut = 0;
}

boolean CUSBMIDIHostDevice::Configure (void)
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
		if ((pEndpointDesc->bmAttributes & 0x3E) != 0x02)	// Bulk or Interrupt EP
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
			m_usBufferSize -=   pEndpointDesc->wMaxPacketSize
					  % CUSBMIDIDevice::EventPacketSize;

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

	// attach interface device
	assert (m_pInterface == 0);
	m_pInterface = new CUSBMIDIDevice;
	assert (m_pInterface != 0);
	m_pInterface->RegisterSendEventsHandler (SendEventsHandler, this);

	return StartRequest ();
}

boolean CUSBMIDIHostDevice::SendEventsHandler (const u8 *pData, unsigned nLength, void *pParam)
{
	CUSBMIDIHostDevice *pThis = static_cast<CUSBMIDIHostDevice *> (pParam);
	assert (pThis);

	assert (pData != 0);
	assert (nLength > 0);
	assert ((nLength & 3) == 0);

	if (pThis->m_pEndpointOut == 0)
	{
		return FALSE;
	}

	DMA_BUFFER (u8, Buffer, nLength);
	memcpy (Buffer, pData, nLength);

	return pThis->GetHost ()->Transfer (pThis->m_pEndpointOut, Buffer, nLength) >= 0;
}

boolean CUSBMIDIHostDevice::StartRequest (void)
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

void CUSBMIDIHostDevice::CompletionRoutine (CUSBRequest *pURB)
{
	assert (pURB != 0);
	assert (m_pInterface != 0);

	boolean bRestart = FALSE;

	if (   pURB->GetStatus () != 0
	    && pURB->GetResultLength () % CUSBMIDIDevice::EventPacketSize == 0)
	{
		assert (m_pPacketBuffer != 0);

		bRestart = m_pInterface->CallPacketHandler (m_pPacketBuffer,
							    pURB->GetResultLength ());
	}
	else if (   m_pInterface->GetAllSoundOffOnUSBError ()
		 && !pURB->GetStatus ()
		 && pURB->GetUSBError () != USBErrorUnknown)
	{
		for (u8 nChannel = 0; nChannel < 16; nChannel++)
		{
			u8 AllSoundOff[] = {0x0B, (u8) (0xB0 | nChannel), 120, 0};
			m_pInterface->CallPacketHandler (AllSoundOff,  sizeof AllSoundOff);
		}
	}

	delete pURB;

	if (   bRestart
	    || CKernelOptions::Get ()->GetUSBBoost ())
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

void CUSBMIDIHostDevice::CompletionStub (CUSBRequest *pURB, void *pParam, void *pContext)
{
	CUSBMIDIHostDevice *pThis = (CUSBMIDIHostDevice *) pContext;
	assert (pThis != 0);

	pThis->CompletionRoutine (pURB);
}

void CUSBMIDIHostDevice::TimerHandler (TKernelTimerHandle hTimer)
{
	assert (m_hTimer == hTimer);
	m_hTimer = 0;

	StartRequest ();
}

void CUSBMIDIHostDevice::TimerStub (TKernelTimerHandle hTimer, void *pParam, void *pContext)
{
	CUSBMIDIHostDevice *pThis = (CUSBMIDIHostDevice *) pContext;
	assert (pThis != 0);

	pThis->TimerHandler (hTimer);
}
