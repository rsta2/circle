//
// usbmidihost.h
//
// Ported from the USPi driver which is:
// 	Copyright (C) 2016  J. Otto <joshua.t.otto@gmail.com>
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2022  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_usbmidihost_h
#define _circle_usb_usbmidihost_h

#include <circle/usb/usbfunction.h>
#include <circle/usb/usbendpoint.h>
#include <circle/usb/usbrequest.h>
#include <circle/usb/usbmidi.h>
#include <circle/timer.h>
#include <circle/types.h>

class CUSBMIDIHostDevice : public CUSBFunction	/// Host driver for USB Audio Class MIDI 1.0 devices
{
public:
	CUSBMIDIHostDevice (CUSBFunction *pFunction);
	~CUSBMIDIHostDevice (void);

	boolean Configure (void);

	static boolean SendEventsHandler (const u8 *pData, unsigned nLength, void *pParam);

private:
	boolean StartRequest (void);

	void CompletionRoutine (CUSBRequest *pURB);
	static void CompletionStub (CUSBRequest *pURB, void *pParam, void *pContext);

	void TimerHandler (TKernelTimerHandle hTimer);
	static void TimerStub (TKernelTimerHandle hTimer, void *pParam, void *pContext);

private:
	CUSBMIDIDevice *m_pInterface;

	CUSBEndpoint *m_pEndpointIn;
	CUSBEndpoint *m_pEndpointOut;

	u16 m_usBufferSize;
	u8 *m_pPacketBuffer;

	TKernelTimerHandle m_hTimer;
};

#endif
