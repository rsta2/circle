//
// usbmidi.h
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
//
#ifndef _circle_usb_usbmidi_h
#define _circle_usb_usbmidi_h

#include <circle/usb/usbfunction.h>
#include <circle/usb/usbendpoint.h>
#include <circle/usb/usbrequest.h>
#include <circle/types.h>

typedef void TMIDIPacketHandler (unsigned nCable, u8 *pPacket, unsigned nLength);

class CUSBMIDIDevice : public CUSBFunction
{
public:
	CUSBMIDIDevice (CUSBFunction *pFunction);
	~CUSBMIDIDevice (void);

	boolean Configure (void);

	void RegisterPacketHandler (TMIDIPacketHandler *pPacketHandler);

private:
	boolean StartRequest (void);

	void CompletionRoutine (CUSBRequest *pURB);
	static void CompletionStub (CUSBRequest *pURB, void *pParam, void *pContext);

	void TimerHandler (unsigned hTimer);
	static void TimerStub (unsigned hTimer, void *pParam, void *pContext);

private:
	CUSBEndpoint *m_pEndpointIn;

	TMIDIPacketHandler *m_pPacketHandler;

	CUSBRequest *m_pURB;
	u16 m_usBufferSize;
	u8 *m_pPacketBuffer;

	unsigned m_hTimer;

	static unsigned s_nDeviceNumber;
};

#endif
