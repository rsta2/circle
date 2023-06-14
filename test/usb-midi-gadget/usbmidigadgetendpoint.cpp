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
#include "usbmidigadgetendpoint.h"
#include "usbmidigadget.h"
#include <circle/logger.h>
#include <circle/debug.h>
#include <circle/util.h>
#include <assert.h>

LOGMODULE ("midigadgetep");

CUSBMIDIGadgetEndpoint::CUSBMIDIGadgetEndpoint (const TUSBEndpointDescriptor *pDesc,
						CUSBMIDIGadget *pGadget)
:	CDWUSBGadgetEndpoint (pDesc, pGadget),
	m_bNoteOn (TRUE),
	m_uchNote (0)
{
}

CUSBMIDIGadgetEndpoint::~CUSBMIDIGadgetEndpoint (void)
{
}

void CUSBMIDIGadgetEndpoint::OnActivate (void)
{
	if (GetDirection () == DirectionOut)
	{
		BeginTransfer (TransferDataOut, m_OutBuffer, MaxMessageSize);
	}
	else
	{
		TimerHandler ();
	}
}

void CUSBMIDIGadgetEndpoint::OnTransferComplete (boolean bIn, size_t nLength)
{
	if (!bIn)
	{
#ifndef NDEBUG
		debug_hexdump (m_OutBuffer, nLength, From);
#endif

		BeginTransfer (TransferDataOut, m_OutBuffer, MaxMessageSize);
	}
	else
	{
		assert (nLength == 4);

		CTimer::Get ()->StartKernelTimer (MSEC2HZ (500), TimerHandler, 0, this);
	}
}

void CUSBMIDIGadgetEndpoint::TimerHandler (void)
{
	static const size_t EventSize = 4;

	if (m_bNoteOn)
	{
		const u8 NoteOn[EventSize] = {0x09, 0x90, m_uchNote, 0x64};

		memcpy (m_InBuffer, NoteOn, EventSize);
	}
	else
	{
		const u8 NoteOff[EventSize] = {0x08, 0x80, m_uchNote, 0x00};
		if (++m_uchNote > 0x7F)
		{
			m_uchNote = 0;
		}

		memcpy (m_InBuffer, NoteOff, EventSize);
	}

	m_bNoteOn = !m_bNoteOn;

	BeginTransfer (TransferDataIn, m_InBuffer, EventSize);
}

void CUSBMIDIGadgetEndpoint::TimerHandler (TKernelTimerHandle hTimer, void *pParam, void *pContext)
{
	CUSBMIDIGadgetEndpoint *pThis = static_cast<CUSBMIDIGadgetEndpoint *> (pContext);
	assert (pThis);

	pThis->TimerHandler ();
}
