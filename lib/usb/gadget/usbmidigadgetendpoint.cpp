//
// usbmidigadgetendpoint.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2023  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/gadget/usbmidigadgetendpoint.h>
#include <circle/usb/gadget/usbmidigadget.h>
#include <circle/usb/usbmidi.h>
#include <circle/sched/scheduler.h>
#include <circle/sysconfig.h>
#include <circle/logger.h>
#include <circle/atomic.h>
#include <circle/debug.h>
#include <circle/util.h>
#include <assert.h>

LOGMODULE ("midigadgetep");

CUSBMIDIGadgetEndpoint::CUSBMIDIGadgetEndpoint (const TUSBEndpointDescriptor *pDesc,
						CUSBMIDIDevice *pInterface,
						CUSBMIDIGadget *pGadget)
:	CDWUSBGadgetEndpoint (pDesc, pGadget),
	m_pInterface (pInterface),
	m_nInBytesRemaining (0)
{
	if (GetDirection () == DirectionIn)
	{
		assert (m_pInterface);
		m_pInterface->RegisterSendEventsHandler (SendEventsHandler, this);
	}
}

CUSBMIDIGadgetEndpoint::~CUSBMIDIGadgetEndpoint (void)
{
}

void CUSBMIDIGadgetEndpoint::OnActivate (void)
{
	if (GetDirection () == DirectionOut)
	{
		BeginTransfer (TransferDataOut, m_OutBuffer, MaxOutMessageSize);
	}
}

void CUSBMIDIGadgetEndpoint::OnTransferComplete (boolean bIn, size_t nLength)
{
	if (!bIn)
	{
#ifndef NDEBUG
		//debug_hexdump (m_OutBuffer, nLength, From);
#endif

		if (!(nLength % CUSBMIDIDevice::EventPacketSize))
		{
			assert (m_pInterface);
			m_pInterface->CallPacketHandler (m_OutBuffer, nLength);
		}

		BeginTransfer (TransferDataOut, m_OutBuffer, MaxOutMessageSize);
	}
	else
	{
		AtomicSub (&m_nInBytesRemaining, (int) nLength);
	}
}

boolean CUSBMIDIGadgetEndpoint::SendEventsHandler (const u8 *pData, unsigned nLength)
{
	assert (pData != 0);
	assert (nLength > 0);
	assert (!(nLength % CUSBMIDIDevice::EventPacketSize));

	if (nLength > MaxInMessageSize)
	{
		LOGWARN ("Trying to send %u bytes (max %lu)", nLength, MaxInMessageSize);

		return FALSE;
	}

	memcpy (m_InBuffer, pData, nLength);

#ifndef NDEBUG
	int nInBytesRemaining =
#endif
		AtomicExchange (&m_nInBytesRemaining, (int) nLength);
	assert (!nInBytesRemaining);

	BeginTransfer (TransferDataIn, m_InBuffer, nLength);

	while (AtomicGet (&m_nInBytesRemaining))
	{
#ifdef NO_BUSY_WAIT
		CScheduler::Get ()->Yield ();
#endif
	}

	return TRUE;
}

boolean CUSBMIDIGadgetEndpoint::SendEventsHandler (const u8 *pData, unsigned nLength, void *pParam)
{
	CUSBMIDIGadgetEndpoint *pThis = static_cast<CUSBMIDIGadgetEndpoint *> (pParam);
	assert (pThis);

	return pThis->SendEventsHandler (pData, nLength);
}
