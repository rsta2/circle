//
// usbgamepad8bitdopro.cpp
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
#include <circle/usb/usbgamepad8bitdopro.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/logger.h>
#include <circle/macros.h>

static const char FromUSBPad8BitDoPro[] = "usbpad8bitdopro";

CUSBGamePad8BitDoProDevice::CUSBGamePad8BitDoProDevice (CUSBFunction *pFunction)
: CUSBGamePad8bitdoDevice (pFunction),
	m_bInterfaceOK (SelectInterfaceByClass (0xFF, 0x5D, 0x01, 1))
{
}

CUSBGamePad8BitDoProDevice::~CUSBGamePad8BitDoProDevice (void)
{
}

boolean CUSBGamePad8BitDoProDevice::Configure (void)
{
	if (!m_bInterfaceOK)
	{
		ConfigurationError (FromUSBPad8BitDoPro);

		return FALSE;
	}

	if (!CUSBGamePad8bitdoDevice::Configure ())
	{
		return FALSE;
	}

	// Vendor specific command to initialize the receiver 
    DMA_BUFFER (u8, Command, 3) = {0x01, 0x03, 0x02};
	if (!SendToEndpointOut (Command, 3))
	{
		CLogger::Get ()->Write (FromUSBPad8BitDoPro, LogError, 
                                "Cannot initialize receiver");

		return FALSE;
	}

	DMA_BUFFER (u8, Response, 20);
	int nResult = GetHost ()->ControlMessage (
		GetEndpoint0 (), REQUEST_IN | REQUEST_VENDOR | REQUEST_TO_INTERFACE,
		1, 0x0100, GetInterfaceNumber (), Response, 20);
	if (nResult != 20)
	{
		CLogger::Get ()->Write (FromUSBPad8BitDoPro, LogError,
					            "Cannot read receiver state (%d byte(s))", nResult);

		return FALSE;
	}

	return StartRequest ();
}
