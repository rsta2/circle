//
// usbgamepad8bitdoxinput.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2026  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/usbgamepad8bitdoxinput.h>
#include <circle/usb/usbdevice.h>

static const char FromUSBPad8BitDoXInput[] = "usbpad8bitdoxinput";

CUSBGamePad8BitDoXInputDevice::CUSBGamePad8BitDoXInputDevice (CUSBFunction *pFunction)
: CUSBGamePad8bitdoDevice (pFunction)
{
}

CUSBGamePad8BitDoXInputDevice::~CUSBGamePad8BitDoXInputDevice (void)
{
}

boolean CUSBGamePad8BitDoXInputDevice::Configure (void)
{
	if (!CUSBGamePad8bitdoDevice::Configure ())
	{
		return FALSE;
	}

	DMA_BUFFER(u8, Command, 3) = {0x01, 0x03, 0x02};
	if (!SendToEndpointOut(Command, 3)) {
		CLogger::Get ()->Write (FromUSBPad8BitDoXInput, LogError, 
                                "Cannot initialize receiver");
		return FALSE;
	}

	const TUSBDeviceDescriptor *pDeviceDesc = GetDevice ()->GetDeviceDescriptor ();
	assert (pDeviceDesc != 0);
	if (pDeviceDesc->bcdDevice == 0x0100)
	{
		// Receiver firmware 1.00 is a bootstrap personality. After this init
		// command it disconnects and re-enumerates as the idle 3107 receiver,
		// or as runtime 3106 once a controller connects. Do not start interrupt
		// polling or issue the runtime status request before that transition.
		CLogger::Get ()->Write (FromUSBPad8BitDoXInput, LogDebug,
						"Waiting for receiver re-enumeration");
		return FALSE;
	}

	if (!StartRequest()) {
	    return FALSE;
	}

	DMA_BUFFER(u8, Response, 20);
	GetHost()->ControlMessage(
		GetEndpoint0(),
		REQUEST_IN | REQUEST_VENDOR | REQUEST_TO_INTERFACE,
		0x01, 0x0100, GetInterfaceNumber(), Response, 20);

	return TRUE;
}
