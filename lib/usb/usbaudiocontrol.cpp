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
#include <circle/util.h>
#include <assert.h>

LOGMODULE ("uaudioctl");
static const char DevicePrefix[] = "uaudioctl";

CNumberPool CUSBAudioControlDevice::s_DeviceNumberPool (1);

CUSBAudioControlDevice::CUSBAudioControlDevice (CUSBFunction *pFunction)
:	CUSBFunction (pFunction),
	m_nDeviceNumber (0)
{
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

	if (!m_Topology.Parse (pCtlIfaceDesc))
	{
		return FALSE;
	}

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

u16 CUSBAudioControlDevice::GetTerminalType (u8 uchInputTerminalID) const
{
	CUSBAudioEntity *pEntity = m_Topology.GetEntity (uchInputTerminalID);
	if (   !pEntity
	    || pEntity->GetEntityType () != CUSBAudioEntity::EntityTerminal)
	{
		return USB_AUDIO_TERMINAL_TYPE_USB_UNDEFINED;
	}

	CUSBAudioTerminal *pInputTerminal = static_cast <CUSBAudioTerminal *> (pEntity);
	CUSBAudioTerminal *pOutputTerminal = m_Topology.FindOutputTerminal (pInputTerminal);
	if (!pOutputTerminal)
	{
		return USB_AUDIO_TERMINAL_TYPE_USB_UNDEFINED;
	}

	return pOutputTerminal->GetTerminalType ();
}

u8 CUSBAudioControlDevice::GetClockSourceID (u8 uchInputTerminalID) const
{
	CUSBAudioEntity *pEntity = m_Topology.GetEntity (uchInputTerminalID);
	if (   !pEntity
	    || pEntity->GetEntityType () != CUSBAudioEntity::EntityTerminal)
	{
		return USB_AUDIO_UNDEFINED_UNIT_ID;
	}

	CUSBAudioTerminal *pTerminal = static_cast <CUSBAudioTerminal *> (pEntity);
	if (!pTerminal->IsInput ())
	{
		return USB_AUDIO_UNDEFINED_UNIT_ID;
	}

	return pTerminal->GetClockSourceID ();
}

u8 CUSBAudioControlDevice::GetFeatureUnitID (u8 uchInputTerminalID) const
{
	CUSBAudioEntity *pEntity = m_Topology.GetEntity (uchInputTerminalID);
	if (   !pEntity
	    || pEntity->GetEntityType () != CUSBAudioEntity::EntityTerminal)
	{
		return USB_AUDIO_UNDEFINED_UNIT_ID;
	}

	CUSBAudioTerminal *pTerminal = static_cast <CUSBAudioTerminal *> (pEntity);
	CUSBAudioFeatureUnit *pUnit = m_Topology.FindFeatureUnit (pTerminal);
	if (!pUnit)
	{
		return USB_AUDIO_UNDEFINED_UNIT_ID;
	}

	return pUnit->GetID ();
}
