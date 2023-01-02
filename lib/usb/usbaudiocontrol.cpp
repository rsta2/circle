//
// usbaudiocontrol.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2022-2023  R. Stange <rsta2@o2online.de>
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
	m_nDeviceNumber (0),
	m_nNextStreamingSubDeviceNumber (1)
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

unsigned CUSBAudioControlDevice::GetDeviceNumber (void) const
{
	assert (m_nDeviceNumber);

	return m_nDeviceNumber;
}

unsigned CUSBAudioControlDevice::GetNextStreamingSubDeviceNumber (void)
{
	return m_nNextStreamingSubDeviceNumber++;
}

u16 CUSBAudioControlDevice::GetTerminalType (u8 uchEntityID, boolean bUpstream) const
{
	CUSBAudioEntity *pEntity = m_Topology.GetEntity (uchEntityID);
	if (!pEntity)
	{
		return USB_AUDIO_TERMINAL_TYPE_USB_UNDEFINED;
	}

	pEntity = m_Topology.FindEntity (CUSBAudioEntity::EntityTerminal, pEntity, bUpstream);
	if (!pEntity)
	{
		return USB_AUDIO_TERMINAL_TYPE_USB_UNDEFINED;
	}

	CUSBAudioTerminal *pTerminal = static_cast <CUSBAudioTerminal *> (pEntity);

	return pTerminal->GetTerminalType ();
}

u8 CUSBAudioControlDevice::GetClockSourceID (u8 uchTerminalID) const
{
	CUSBAudioEntity *pEntity = m_Topology.GetEntity (uchTerminalID);
	if (   !pEntity
	    || pEntity->GetEntityType () != CUSBAudioEntity::EntityTerminal)
	{
		return USB_AUDIO_UNDEFINED_UNIT_ID;
	}

	CUSBAudioTerminal *pTerminal = static_cast <CUSBAudioTerminal *> (pEntity);

	u8 uchClockSourceID = pTerminal->GetClockSourceID ();

	CUSBAudioEntity *pClockEntity = m_Topology.GetEntity (uchClockSourceID);
	if (!pClockEntity)
	{
		return USB_AUDIO_UNDEFINED_UNIT_ID;
	}

	if (pClockEntity->GetEntityType () == CUSBAudioEntity::EntityClockSelector)
	{
		uchClockSourceID = pClockEntity->GetSourceID (0);
	}

	return uchClockSourceID;
}

u8 CUSBAudioControlDevice::GetSelectorUnitID (u8 uchOutputTerminalID) const
{
	CUSBAudioEntity *pEntity = m_Topology.GetEntity (uchOutputTerminalID);
	if (   !pEntity
	    || pEntity->GetEntityType () != CUSBAudioEntity::EntityTerminal)
	{
		return USB_AUDIO_UNDEFINED_UNIT_ID;
	}

	pEntity = m_Topology.FindEntity (CUSBAudioEntity::EntitySelectorUnit, pEntity, TRUE);
	if (!pEntity)
	{
		return USB_AUDIO_UNDEFINED_UNIT_ID;
	}

	return pEntity->GetID ();
}

unsigned CUSBAudioControlDevice::GetNumSources (u8 uchEntityID) const
{
	CUSBAudioEntity *pEntity = m_Topology.GetEntity (uchEntityID);
	if (!pEntity)
	{
		return 0;
	}

	return pEntity->GetNumSources ();
}

u8 CUSBAudioControlDevice::GetSourceID (u8 uchEntityID, unsigned nIndex) const
{
	CUSBAudioEntity *pEntity = m_Topology.GetEntity (uchEntityID);
	if (!pEntity)
	{
		return USB_AUDIO_UNDEFINED_UNIT_ID;
	}

	return pEntity->GetSourceID (nIndex);
}

u8 CUSBAudioControlDevice::GetFeatureUnitID (u8 uchEntityID, boolean bUpstream) const
{
	CUSBAudioEntity *pEntity = m_Topology.GetEntity (uchEntityID);
	if (!pEntity)
	{
		return USB_AUDIO_UNDEFINED_UNIT_ID;
	}

	if (pEntity->GetEntityType () == CUSBAudioEntity::EntityFeatureUnit)
	{
		return uchEntityID;
	}

	pEntity = m_Topology.FindEntity (CUSBAudioEntity::EntityFeatureUnit, pEntity, bUpstream);
	if (!pEntity)
	{
		return USB_AUDIO_UNDEFINED_UNIT_ID;
	}

	return pEntity->GetID ();
}

u8 CUSBAudioControlDevice::GetClockSelectorID (unsigned nIndex)
{
	CUSBAudioEntity *pEntity = m_Topology.FindEntity (CUSBAudioEntity::EntityClockSelector,
							  nIndex);
	if (!pEntity)
	{
		return USB_AUDIO_UNDEFINED_UNIT_ID;
	}

	return pEntity->GetID ();
}

boolean CUSBAudioControlDevice::IsControlSupported (u8 uchFeatureUnitID, unsigned nChannel,
						    CUSBAudioFeatureUnit::TControl Control) const
{
	CUSBAudioEntity *pEntity = m_Topology.GetEntity (uchFeatureUnitID);
	if (   !pEntity
	    || pEntity->GetEntityType () != CUSBAudioEntity::EntityFeatureUnit)
	{
		return FALSE;
	}

	CUSBAudioFeatureUnit *pUnit = static_cast <CUSBAudioFeatureUnit *> (pEntity);

	return pUnit->GetControlStatus (nChannel, Control) == CUSBAudioFeatureUnit::ControlReadWrite;
}
