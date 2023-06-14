//
// usbmidigadgetendpoint.h
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
#ifndef _circle_usb_gadget_usbmidigadgetendpoint_h
#define _circle_usb_gadget_usbmidigadgetendpoint_h

#include <circle/usb/gadget/dwusbgadgetendpoint.h>
#include <circle/usb/usb.h>
#include <circle/synchronize.h>
#include <circle/timer.h>
#include <circle/types.h>

class CUSBMIDIGadget;

class CUSBMIDIGadgetEndpoint : public CDWUSBGadgetEndpoint	/// Endpoint of the USB MIDI gadget
{
public:
	CUSBMIDIGadgetEndpoint (const TUSBEndpointDescriptor *pDesc, CUSBMIDIGadget *pGadget);
	~CUSBMIDIGadgetEndpoint (void);

	void OnActivate (void) override;

	void OnTransferComplete (boolean bIn, size_t nLength) override;

private:
	void TimerHandler (void);
	static void TimerHandler (TKernelTimerHandle hTimer, void *pParam, void *pContext);

private:
	static const size_t MaxMessageSize = 64;
	DMA_BUFFER (u8, m_OutBuffer, MaxMessageSize);
	DMA_BUFFER (u8, m_InBuffer, MaxMessageSize);

	boolean m_bNoteOn;
	u8 m_uchNote;
};

#endif
