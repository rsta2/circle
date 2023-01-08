//
// usbaudiofunctopology.cpp
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
#include <circle/usb/usbaudiofunctopology.h>
#include <circle/usb/usb.h>
#include <circle/logger.h>
#include <circle/string.h>
#include <circle/macros.h>
#include <circle/util.h>
#include <assert.h>

LOGMODULE ("uaudiotopo");

////////////////////////////////////////////////////////////////////////////////

CUSBAudioEntity::CUSBAudioEntity (TEntityType Type)
:	m_EntityType (Type),
	m_ID (UndefinedID),
	m_nSources (0),
	m_nChannels (0)
{
	memset (m_SourceID, UndefinedID, sizeof m_SourceID);
}

CUSBAudioEntity::TEntityType CUSBAudioEntity::GetEntityType (void) const
{
	return m_EntityType;
}

void CUSBAudioEntity::SetID (TEntityID ID)
{
	assert (ID != UndefinedID);

	m_ID = ID;
}

CUSBAudioEntity::TEntityID CUSBAudioEntity::GetID (void) const
{
	return m_ID;
}

void CUSBAudioEntity::SetNumSources (unsigned nSources)
{
	assert (nSources <= MaximumSourceIndex+1);

	m_nSources = nSources;
}

void CUSBAudioEntity::SetSourceID (unsigned Index, TEntityID ID)
{
	assert (Index <= m_nSources);
	assert (m_SourceID[Index] == UndefinedID);

	m_SourceID[Index] = ID;
}

void CUSBAudioEntity::SetSourceID (TEntityID ID)
{
	SetNumSources (1);
	SetSourceID (0, ID);
}

unsigned CUSBAudioEntity::GetNumSources (void) const
{
	return m_nSources;
}

CUSBAudioEntity::TEntityID CUSBAudioEntity::GetSourceID (unsigned Index) const
{
	assert (Index <= m_nSources);

	return m_SourceID[Index];
}

void CUSBAudioEntity::SetNumChannels (unsigned nChannels)
{
	assert (nChannels <= MaximumChannelIndex);

	m_nChannels = nChannels;
}

unsigned CUSBAudioEntity::GetNumChannels (void) const
{
	assert (m_nChannels <= MaximumChannelIndex);

	return m_nChannels;
}

void CUSBAudioEntity::Dump (void)
{
	CString Sources (", src ");
	for (unsigned i = 0; i < m_nSources; i++)
	{
		CString String;
		String.Format ("%u", (unsigned) m_SourceID[i]);

		if (i)
		{
			Sources.Append (", ");
		}

		Sources.Append (String);
	}

	if (Sources.GetLength () <= 6)
	{
		Sources = "";
	}

	LOGDBG ("Entity #%u (type %u, %u chans%s)",
		m_ID, m_EntityType, m_nChannels, (const char *) Sources);
}

////////////////////////////////////////////////////////////////////////////////

CUSBAudioTerminal::CUSBAudioTerminal (const TUSBAudioControlInterfaceDescriptor *pDesc,
				      boolean bVer200)
:	CUSBAudioEntity (EntityTerminal),
	m_ClockSourceID (UndefinedID)
{
	assert (pDesc);

	m_bIsInput = pDesc->bDescriptorSubtype == USB_AUDIO_CTL_IFACE_SUBTYPE_INPUT_TERMINAL;

	if (m_bIsInput)
	{
		if (bVer200)
		{
			SetID (pDesc->Ver200.InputTerminal.bTerminalID);

			m_usTerminalType = pDesc->Ver200.InputTerminal.wTerminalType;
			m_ClockSourceID = pDesc->Ver200.InputTerminal.bCSourceID;

			SetNumChannels (pDesc->Ver200.InputTerminal.bNrChannels);
		}
		else
		{
			SetID (pDesc->Ver100.InputTerminal.bTerminalID);

			m_usTerminalType = pDesc->Ver100.InputTerminal.wTerminalType;

			SetNumChannels (pDesc->Ver100.InputTerminal.bNrChannels);
		}

		if (   GetNumChannels () == 0
		    && m_usTerminalType == USB_AUDIO_TERMINAL_TYPE_USB_STREAMING)
		{
			SetNumChannels (2);
		}
	}
	else
	{
		assert (pDesc->bDescriptorSubtype == USB_AUDIO_CTL_IFACE_SUBTYPE_OUTPUT_TERMINAL);

		if (bVer200)
		{
			SetID (pDesc->Ver200.OutputTerminal.bTerminalID);
			SetSourceID (pDesc->Ver200.OutputTerminal.bSourceID);

			m_usTerminalType = pDesc->Ver200.OutputTerminal.wTerminalType;
			m_ClockSourceID = pDesc->Ver200.OutputTerminal.bCSourceID;
		}
		else
		{
			SetID (pDesc->Ver100.OutputTerminal.bTerminalID);
			SetSourceID (pDesc->Ver100.OutputTerminal.bSourceID);

			m_usTerminalType = pDesc->Ver100.OutputTerminal.wTerminalType;
		}
	}
}

boolean CUSBAudioTerminal::IsInput (void) const
{
	return m_bIsInput;
}

u16 CUSBAudioTerminal::GetTerminalType (void) const
{
	return m_usTerminalType;
}

CUSBAudioEntity::TEntityID CUSBAudioTerminal::GetClockSourceID (void) const
{
	return m_ClockSourceID;
}

////////////////////////////////////////////////////////////////////////////////

CUSBAudioMixerUnit::CUSBAudioMixerUnit (const TUSBAudioControlInterfaceDescriptor *pDesc,
					boolean bVer200)
:	CUSBAudioEntity (EntityMixerUnit)
{
	assert (pDesc);
	assert (pDesc->bDescriptorSubtype == USB_AUDIO_CTL_IFACE_SUBTYPE_MIXER_UNIT);

	if (bVer200)
	{
		SetID (pDesc->Ver200.MixerUnit.bUnitID);

		SetNumSources (pDesc->Ver200.MixerUnit.bNrInPins);
		for (unsigned i = 0; i < GetNumSources (); i++)
		{
			SetSourceID (i, pDesc->Ver200.MixerUnit.baSourceID[i]);
		}

		const TUSBAudioControlMixerUnitTrailerVer200 *pTrailer =
			reinterpret_cast<const TUSBAudioControlMixerUnitTrailerVer200 *> (
				&pDesc->Ver200.MixerUnit.baSourceID[GetNumSources ()]);

		SetNumChannels (pTrailer->bNrChannels);
	}
	else
	{
		SetID (pDesc->Ver100.MixerUnit.bUnitID);

		SetNumSources (pDesc->Ver100.MixerUnit.bNrInPins);
		for (unsigned i = 0; i < GetNumSources (); i++)
		{
			SetSourceID (i, pDesc->Ver100.MixerUnit.baSourceID[i]);
		}

		const TUSBAudioControlMixerUnitTrailerVer100 *pTrailer =
			reinterpret_cast<const TUSBAudioControlMixerUnitTrailerVer100 *> (
				&pDesc->Ver100.MixerUnit.baSourceID[GetNumSources ()]);

		SetNumChannels (pTrailer->bNrChannels);
	}
}

////////////////////////////////////////////////////////////////////////////////

CUSBAudioSelectorUnit::CUSBAudioSelectorUnit (const TUSBAudioControlInterfaceDescriptor *pDesc,
					      boolean bVer200)
:	CUSBAudioEntity (EntitySelectorUnit)
{
	assert (pDesc);
	assert (pDesc->bDescriptorSubtype == USB_AUDIO_CTL_IFACE_SUBTYPE_SELECTOR_UNIT);

	if (bVer200)
	{
		SetID (pDesc->Ver200.SelectorUnit.bUnitID);

		SetNumSources (pDesc->Ver200.SelectorUnit.bNrInPins);
		for (unsigned i = 0; i < GetNumSources (); i++)
		{
			SetSourceID (i, pDesc->Ver200.SelectorUnit.baSourceID[i]);
		}
	}
	else
	{
		SetID (pDesc->Ver100.SelectorUnit.bUnitID);

		SetNumSources (pDesc->Ver100.SelectorUnit.bNrInPins);
		for (unsigned i = 0; i < GetNumSources (); i++)
		{
			SetSourceID (i, pDesc->Ver100.SelectorUnit.baSourceID[i]);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

CUSBAudioFeatureUnit::CUSBAudioFeatureUnit (const TUSBAudioControlInterfaceDescriptor *pDesc,
					    boolean bVer200)
:	CUSBAudioEntity (EntityFeatureUnit)
{
	assert (pDesc);
	assert (pDesc->bDescriptorSubtype == USB_AUDIO_CTL_IFACE_SUBTYPE_FEATURE_UNIT);

	if (bVer200)
	{
		SetID (pDesc->Ver200.FeatureUnit.bUnitID);
		SetSourceID (pDesc->Ver200.FeatureUnit.bSourceID);

		assert (pDesc->bLength >= 10);
		assert ((pDesc->bLength-10) % 4 == 0);
		SetNumChannels ((pDesc->bLength-10) / 4);

		for (unsigned i = 0; i <= GetNumChannels (); i++)
		{
			m_nControls[i] = pDesc->Ver200.FeatureUnit.bmaControls[i];
		}
	}
	else
	{
		SetID (pDesc->Ver100.FeatureUnit.bUnitID);
		SetSourceID (pDesc->Ver100.FeatureUnit.bSourceID);

		unsigned nControlSize = pDesc->Ver100.FeatureUnit.bControlSize;
		assert (nControlSize <= MaximumControl);

		assert (pDesc->bLength >= 7+nControlSize);
		SetNumChannels ((pDesc->bLength-7-nControlSize) / nControlSize);

		nControlSize *= 8;
		for (unsigned i = 0; i <= GetNumChannels (); i++)
		{
			u32 nControls = 0;
			for (unsigned j = 0; j < nControlSize; j++)
			{
				u16 bmaControls;
				if (nControlSize <= 8)
				{
					bmaControls = pDesc->Ver100.FeatureUnit.bmaControls[i];
				}
				else
				{
					bmaControls =
						  pDesc->Ver100.FeatureUnit.bmaControls[i+i]
						| pDesc->Ver100.FeatureUnit.bmaControls[i+i+1] << 8;
				}

				if (bmaControls & BIT (j))
				{
					nControls |= ControlReadWrite << (j+j);
				}
			}

			m_nControls[i] = nControls;
		}
	}
}

CUSBAudioFeatureUnit::TControlStatus CUSBAudioFeatureUnit::GetControlStatus (unsigned nChannel,
									     TControl Control) const
{
	assert (nChannel <= MaximumChannelIndex);
	assert (Control < MaximumControl);

	return static_cast<TControlStatus> ((m_nControls[nChannel] >> (Control*2)) & 3);
}

////////////////////////////////////////////////////////////////////////////////

CUSBAudioClockSource::CUSBAudioClockSource (const TUSBAudioControlInterfaceDescriptor *pDesc,
					    boolean bVer200)
:	CUSBAudioEntity (EntityClockSource)
{
	assert (pDesc);
	assert (pDesc->bDescriptorSubtype == USB_AUDIO_CTL_IFACE_SUBTYPE_CLOCK_SOURCE);
	assert (bVer200);

	SetID (pDesc->Ver200.ClockSource.bClockID);
}

////////////////////////////////////////////////////////////////////////////////

CUSBAudioClockSelector::CUSBAudioClockSelector (const TUSBAudioControlInterfaceDescriptor *pDesc,
						boolean bVer200)
:	CUSBAudioEntity (EntityClockSelector)
{
	assert (pDesc);
	assert (pDesc->bDescriptorSubtype == USB_AUDIO_CTL_IFACE_SUBTYPE_CLOCK_SELECTOR);
	assert (bVer200);

	SetID (pDesc->Ver200.ClockSelector.bClockID);

	SetNumSources (pDesc->Ver200.ClockSelector.bNrInPins);
	for (unsigned i = 0; i < GetNumSources (); i++)
	{
		SetSourceID (i, pDesc->Ver200.ClockSelector.baCSourceID[i]);
	}
}

////////////////////////////////////////////////////////////////////////////////

CUSBAudioFunctionTopology::CUSBAudioFunctionTopology (void)
{
	memset (m_pEntity, 0, sizeof m_pEntity);
}

CUSBAudioFunctionTopology::~CUSBAudioFunctionTopology (void)
{
	for (unsigned i = 0; i <= CUSBAudioEntity::MaximumID; i++)
	{
		delete m_pEntity[i];
		m_pEntity[i] = nullptr;
	}
}

// TODO: Parser fails with assertion, if device configuration is wrong
boolean CUSBAudioFunctionTopology::Parse (
	const TUSBAudioControlInterfaceDescriptor *pDescriptorHeader)
{
	const TUSBAudioControlInterfaceDescriptor *pDesc = pDescriptorHeader;
	assert (pDesc);

	while (pDesc->bDescriptorType == DESCRIPTOR_CS_INTERFACE)
	{
		CUSBAudioEntity *pEntity = nullptr;

		switch (pDesc->bDescriptorSubtype)
		{
		case USB_AUDIO_CTL_IFACE_SUBTYPE_HEADER:
			m_bVer200 = pDesc->Ver200.Header.bcdADC == USB_AUDIO_CTL_IFACE_BCDADC_200;
			if (   !m_bVer200
			    && pDesc->Ver100.Header.bcdADC != USB_AUDIO_CTL_IFACE_BCDADC_100)
			{
				LOGWARN ("Unsupported audio device class version");

				return FALSE;
			}

			// If nothing follows, this is a MIDI function, which is not supported here.
			if ((m_bVer200 ? pDesc->Ver200.Header.wTotalLength
				       : pDesc->Ver100.Header.wTotalLength) <= pDesc->bLength)
			{
				return FALSE;
			}
			break;

		case USB_AUDIO_CTL_IFACE_SUBTYPE_INPUT_TERMINAL:
		case USB_AUDIO_CTL_IFACE_SUBTYPE_OUTPUT_TERMINAL:
			pEntity = new CUSBAudioTerminal (pDesc, m_bVer200);
			assert (pEntity);
			break;

		case USB_AUDIO_CTL_IFACE_SUBTYPE_MIXER_UNIT:
			pEntity = new CUSBAudioMixerUnit (pDesc, m_bVer200);
			assert (pEntity);
			break;

		case USB_AUDIO_CTL_IFACE_SUBTYPE_SELECTOR_UNIT:
			pEntity = new CUSBAudioSelectorUnit (pDesc, m_bVer200);
			assert (pEntity);
			break;

		case USB_AUDIO_CTL_IFACE_SUBTYPE_FEATURE_UNIT:
			pEntity = new CUSBAudioFeatureUnit (pDesc, m_bVer200);
			assert (pEntity);
			break;

		case USB_AUDIO_CTL_IFACE_SUBTYPE_CLOCK_SOURCE:
			pEntity = new CUSBAudioClockSource (pDesc, m_bVer200);
			assert (pEntity);
			break;

		case USB_AUDIO_CTL_IFACE_SUBTYPE_CLOCK_SELECTOR:
			pEntity = new CUSBAudioClockSelector (pDesc, m_bVer200);
			assert (pEntity);
			break;

		default:
			LOGWARN ("Unsupported entity (%u)", (unsigned) pDesc->bDescriptorSubtype);
			break;
		}

		if (pEntity)
		{
			CUSBAudioEntity::TEntityID ID = pEntity->GetID ();
			if (ID == CUSBAudioEntity::UndefinedID)
			{
				LOGWARN ("Cannot register entity");
				pEntity->Dump ();

				delete pEntity;

				return FALSE;
			}

			assert (!m_pEntity[ID]);
			m_pEntity[ID] = pEntity;
		}

		pDesc = (TUSBAudioControlInterfaceDescriptor *) ((u8 *) pDesc + pDesc->bLength);
	}

	//Dump ();

	return TRUE;
}

CUSBAudioEntity *CUSBAudioFunctionTopology::GetEntity (CUSBAudioEntity::TEntityID ID) const
{
	return m_pEntity[ID];
}

CUSBAudioEntity *CUSBAudioFunctionTopology::FindEntity (CUSBAudioEntity::TEntityType EntityType,
							CUSBAudioEntity *pStartFromEntity,
							boolean bUpstream) const
{
	assert (pStartFromEntity);

	for (unsigned i = 0; i <= CUSBAudioEntity::MaximumID; i++)
	{
		if (   !m_pEntity[i]
		    || m_pEntity[i]->GetEntityType () != EntityType)
		{
			continue;
		}

		if (bUpstream)
		{
			if (FindUpstream (pStartFromEntity, m_pEntity[i]))
			{
				return m_pEntity[i];
			}
		}
		else
		{
			if (FindUpstream (m_pEntity[i], pStartFromEntity))
			{
				return m_pEntity[i];
			}
		}
	}

	return nullptr;
}

CUSBAudioEntity *CUSBAudioFunctionTopology::FindEntity (CUSBAudioEntity::TEntityType EntityType,
							unsigned nIndex) const
{
	for (unsigned i = 0; i <= CUSBAudioEntity::MaximumID; i++)
	{
		if (   !m_pEntity[i]
		    || m_pEntity[i]->GetEntityType () != EntityType)
		{
			continue;
		}

		if (nIndex-- == 0)
		{
			return m_pEntity[i];
		}
	}

	return nullptr;
}

boolean CUSBAudioFunctionTopology::FindUpstream (CUSBAudioEntity *pCurrentEntity,
						 CUSBAudioEntity *pEntityToFind) const
{
	assert (pCurrentEntity);
	assert (pEntityToFind);

	for (unsigned i = 0; i < pCurrentEntity->GetNumSources (); i++)
	{
		CUSBAudioEntity *pSource = m_pEntity[pCurrentEntity->GetSourceID (i)];
		if (!pSource)
		{
			LOGWARN ("Source entity not found (%u)", pCurrentEntity->GetSourceID (i));

			continue;
		}

		if (pSource == pEntityToFind)
		{
			return TRUE;
		}

		if (FindUpstream (pSource, pEntityToFind))
		{
			return TRUE;
		}
	}

	return FALSE;
}

void CUSBAudioFunctionTopology::Dump (void)
{
	LOGDBG ("Topology dump:");

	for (unsigned i = 0; i <= CUSBAudioEntity::MaximumID; i++)
	{
		if (m_pEntity[i])
		{
			m_pEntity[i]->Dump ();
		}
	}
}
