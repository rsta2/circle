//
// usbaudiocontrol.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2022  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/usbaudiocontrol.h>
#include <circle/devicenameservice.h>
#include <circle/logger.h>
#include <circle/debug.h>
#include <circle/util.h>
#include <assert.h>

LOGMODULE ("uaudioctl");
static const char DevicePrefix[] = "uaudioctl";

CNumberPool CUSBAudioControlDevice::s_DeviceNumberPool (1);

CUSBAudioControlDevice::CUSBAudioControlDevice (CUSBFunction *pFunction)
:	CUSBFunction (pFunction),
	m_nDeviceNumber (0)
{
	memset (m_ClockSourceID, USB_AUDIO_UNDEFINED_UNIT_ID, sizeof m_ClockSourceID);
}

CUSBAudioControlDevice::~CUSBAudioControlDevice (void)
{
	if (m_nDeviceNumber)
	{
		CDeviceNameService::Get ()->RemoveDevice (DevicePrefix, m_nDeviceNumber, FALSE);

		s_DeviceNumberPool.FreeNumber (m_nDeviceNumber);
	}
}

boolean CUSBAudioControlDevice::Configure (void)
{
	TUSBAudioControlInterfaceDescriptor *pCtlIfaceDesc =
		(TUSBAudioControlInterfaceDescriptor *) GetDescriptor (DESCRIPTOR_CS_INTERFACE);
	if (   !pCtlIfaceDesc
	    || pCtlIfaceDesc->bDescriptorSubtype != USB_AUDIO_CTL_IFACE_SUBTYPE_HEADER)
	{
		LOGWARN ("Control interface descriptor header expected");

		return FALSE;
	}

	// If no additional unit descriptor follows (e.g. this is a MIDI device),
	// silently ignore this USB function.
	if (pCtlIfaceDesc->Ver100.Header.bcdADC == USB_AUDIO_CTL_IFACE_BCDADC_100)
	{
		if (pCtlIfaceDesc->Ver100.Header.wTotalLength <= pCtlIfaceDesc->bLength)
		{
			return FALSE;
		}
	}
	else if (pCtlIfaceDesc->Ver200.Header.bcdADC == USB_AUDIO_CTL_IFACE_BCDADC_200)
	{
		if (pCtlIfaceDesc->Ver200.Header.wTotalLength <= pCtlIfaceDesc->bLength)
		{
			return FALSE;
		}
	}
	else
	{
		LOGWARN ("Unsupported audio device class version");

		return FALSE;
	}

	// Currently we support USB audio class v2.00 only.
	if (pCtlIfaceDesc->Ver200.Header.bcdADC == USB_AUDIO_CTL_IFACE_BCDADC_200)
	{
		// Walk all CS_INTERFACE descriptors for INPUT_TERMINAL descriptors.
		while (pCtlIfaceDesc->bDescriptorType == DESCRIPTOR_CS_INTERFACE)
		{
			if (   pCtlIfaceDesc->bDescriptorSubtype
			    == USB_AUDIO_CTL_IFACE_SUBTYPE_INPUT_TERMINAL)
			{
				assert (   pCtlIfaceDesc->Ver200.InputTerminal.bCSourceID
					!= USB_AUDIO_UNDEFINED_UNIT_ID);
				m_ClockSourceID[pCtlIfaceDesc->Ver200.InputTerminal.bTerminalID] =
					pCtlIfaceDesc->Ver200.InputTerminal.bCSourceID;
			}

			pCtlIfaceDesc = (TUSBAudioControlInterfaceDescriptor *)
						((u8 *) pCtlIfaceDesc + pCtlIfaceDesc->bLength);
		}
	}

	//debug_hexdump (m_ClockSourceID, sizeof m_ClockSourceID, From);

	if (!CUSBFunction::Configure ())
	{
		LOGWARN ("Cannot set interface");

		return FALSE;
	}

	assert (m_nDeviceNumber == 0);
	m_nDeviceNumber = s_DeviceNumberPool.AllocateNumber (TRUE, From);

	CDeviceNameService::Get ()->AddDevice (DevicePrefix, m_nDeviceNumber, this, FALSE);

	return TRUE;
}

u8 CUSBAudioControlDevice::GetClockSourceID (u8 uchInputTerminalID) const
{
	return m_ClockSourceID[uchInputTerminalID];
}
