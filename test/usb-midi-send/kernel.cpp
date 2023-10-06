//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2023  R. Stange <rsta2@o2online.de>
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
#include "kernel.h"
#include <circle/usb/usbhcidevice.h>
#include <circle/usb/gadget/usbmidigadget.h>
#include <assert.h>

//#define USB_GADGET_MODE

LOGMODULE ("kernel");

const u8 CKernel::s_Note[] = {57, 59, 60, 62, 64, 65, 67, 0};

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
#ifndef USB_GADGET_MODE
	m_pUSB (new CUSBHCIDevice (&m_Interrupt, &m_Timer, TRUE)), // TRUE: enable plug-and-play
#else
	m_pUSB (new CUSBMIDIGadget (&m_Interrupt)),
#endif
	m_pMIDIDevice (nullptr)
{
	m_ActLED.Blink (5);	// show we are alive
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;

	if (bOK)
	{
		bOK = m_Screen.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Serial.Initialize (115200);
	}

	if (bOK)
	{
		CDevice *pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
		if (pTarget == 0)
		{
			pTarget = &m_Screen;
		}

		bOK = m_Logger.Initialize (pTarget);
	}

	if (bOK)
	{
		bOK = m_Interrupt.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Timer.Initialize ();
	}

	if (bOK)
	{
		assert (m_pUSB);
		bOK = m_pUSB->Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (From, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	boolean bNoteOn = TRUE;
	unsigned nNoteIndex = 0;
	unsigned nLastTicks = 0;
	
	for (unsigned nCount = 0; 1; nCount++)
	{
		// This must be called from TASK_LEVEL to update the tree of connected USB devices.
		assert (m_pUSB);
		boolean bUpdated = m_pUSB->UpdatePlugAndPlay ();

		if (   bUpdated
		    && !m_pMIDIDevice)
		{
			m_pMIDIDevice = static_cast<CUSBMIDIDevice *> (
				CDeviceNameService::Get ()->GetDevice ("umidi1", FALSE));
			if (m_pMIDIDevice)
			{
				m_pMIDIDevice->RegisterRemovedHandler (DeviceRemovedHandler, this);

				bNoteOn = TRUE;
				nNoteIndex = 0;
				nLastTicks = m_Timer.GetTicks ();
			}
		}

		unsigned nTicks = m_Timer.GetTicks ();

		if (   m_pMIDIDevice
		    && nTicks - nLastTicks >= MSEC2HZ (500))
		{
			nLastTicks = nTicks;

			if (bNoteOn)
			{
				const u8 NoteOn[] = {0x90, s_Note[nNoteIndex], 100};

				if (!m_pMIDIDevice->SendPlainMIDI (0, NoteOn, sizeof NoteOn))
				{
					LOGWARN ("Send failed");
				}
			}
			else
			{
				const u8 NoteOff[] = {0x80, s_Note[nNoteIndex], 0};

				if (!m_pMIDIDevice->SendPlainMIDI (0, NoteOff, sizeof NoteOff))
				{
					LOGWARN ("Send failed");
				}

				if (!s_Note[++nNoteIndex])
				{
					nNoteIndex = 0;
				}
			}

			bNoteOn = !bNoteOn;
		}

		m_Screen.Rotor (0, nCount);
	}

	return ShutdownHalt;
}

void CKernel::DeviceRemovedHandler (CDevice *pDevice, void *pContext)
{
	CKernel *pThis = static_cast<CKernel *> (pContext);
	assert (pThis);

	pThis->m_pMIDIDevice = nullptr;
}
