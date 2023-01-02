//
// usbaudiofunctopology.h
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
#ifndef _circle_usb_usbaudiofunctopology_h
#define _circle_usb_usbaudiofunctopology_h

#include <circle/usb/usbaudio.h>
#include <circle/types.h>

class CUSBAudioEntity
{
public:
	enum TEntityType
	{
		EntityTerminal,
		EntityMixerUnit,
		EntitySelectorUnit,
		EntityFeatureUnit,
		EntityClockSource,
		EntityClockSelector,
		EntityUnknown
	};

	typedef u8 TEntityID;
	static const TEntityID UndefinedID = USB_AUDIO_UNDEFINED_UNIT_ID;
	static const TEntityID MaximumID   = USB_AUDIO_MAXIMUM_UNIT_ID;

	static const unsigned MaximumSourceIndex  = 9;		// 0..9, best guess
	static const unsigned MaximumChannelIndex = 100;	// 0: reserved for master channel

public:
	CUSBAudioEntity (TEntityType Type);

	TEntityType GetEntityType (void) const;

	void SetID (TEntityID ID);
	TEntityID GetID (void) const;

	void SetNumSources (unsigned nSources);
	void SetSourceID (unsigned Index, TEntityID ID);
	void SetSourceID (TEntityID ID);			// if one source only
	unsigned GetNumSources (void) const;
	TEntityID GetSourceID (unsigned Index) const;

	void SetNumChannels (unsigned nChannels);
	unsigned GetNumChannels (void) const;

	void Dump (void);

private:
	TEntityType m_EntityType;
	TEntityID m_ID;

	unsigned m_nSources;
	TEntityID m_SourceID[MaximumSourceIndex+1];

	unsigned m_nChannels;
};

class CUSBAudioTerminal : public CUSBAudioEntity
{
public:
	CUSBAudioTerminal (const TUSBAudioControlInterfaceDescriptor *pDesc, boolean bVer200);

	boolean IsInput (void) const;

	u16 GetTerminalType (void) const;
	TEntityID GetClockSourceID (void) const;

private:
	boolean m_bIsInput;

	u16 m_usTerminalType;
	TEntityID m_ClockSourceID;
};

class CUSBAudioMixerUnit : public CUSBAudioEntity
{
public:
	CUSBAudioMixerUnit (const TUSBAudioControlInterfaceDescriptor *pDesc, boolean bVer200);
};

class CUSBAudioSelectorUnit : public CUSBAudioEntity
{
public:
	CUSBAudioSelectorUnit (const TUSBAudioControlInterfaceDescriptor *pDesc, boolean bVer200);
};

class CUSBAudioFeatureUnit : public CUSBAudioEntity
{
public:
	enum TControl
	{
		MuteControl,
		VolumeControl,
		BassControl,
		MidControl,
		TrebleControl,
		GraphicEqualizerControl,
		AutomaticGainControl,
		DelayControl,
		BassBoostControl,
		LoudnessControl,

		// v2.00 only
		InputGainControl,
		InputGainPadControl,
		PhaseInverterControl,
		UnderflowControl,
		OverflowControl,

		ReservedControl,
		MaximumControl
	};

	enum TControlStatus
	{
		ControlNotPresent,
		ControlReadOnly,
		ControlNotAllowed,
		ControlReadWrite
	};

public:
	CUSBAudioFeatureUnit (const TUSBAudioControlInterfaceDescriptor *pDesc, boolean bVer200);

	TControlStatus GetControlStatus (unsigned nChannel, TControl Control) const;

private:
	u32 m_nControls[MaximumChannelIndex+1];
};

class CUSBAudioClockSource : public CUSBAudioEntity
{
public:
	CUSBAudioClockSource (const TUSBAudioControlInterfaceDescriptor *pDesc, boolean bVer200);
};

class CUSBAudioClockSelector : public CUSBAudioEntity
{
public:
	CUSBAudioClockSelector (const TUSBAudioControlInterfaceDescriptor *pDesc, boolean bVer200);
};

class CUSBAudioFunctionTopology		/// Topology parser for USB audio class devices
{
public:
	CUSBAudioFunctionTopology (void);
	~CUSBAudioFunctionTopology (void);

	boolean Parse (const TUSBAudioControlInterfaceDescriptor *pDescriptorHeader);

	/// \return Entity for ID
	CUSBAudioEntity *GetEntity (CUSBAudioEntity::TEntityID ID) const;

	/// \param EntityType Entity type to search for
	/// \param pStartFromEntity Where to start from in the topology tree
	/// \param bUpstream Search upstream (OT -> IT) or downstream (IT -> OT)
	/// \return Pointer to found entity, or nullptr if not found
	CUSBAudioEntity *FindEntity (CUSBAudioEntity::TEntityType EntityType,
				     CUSBAudioEntity *pStartFromEntity,
				     boolean bUpstream) const;

	/// \param EntityType Entity type to search for
	/// \param nIndex 0-based index of the entity to search for (in increasing ID order)
	/// \return Pointer to found entity, or nullptr if not found
	CUSBAudioEntity *FindEntity (CUSBAudioEntity::TEntityType EntityType,
				     unsigned nIndex = 0) const;

private:
	// Is entity *pEntityToFind inside the upstream from *pCurrentEntity
	// to the terminating terminal entity?
	boolean FindUpstream (CUSBAudioEntity *pCurrentEntity, CUSBAudioEntity *pEntityToFind) const;

	void Dump (void);

private:
	boolean m_bVer200;
	CUSBAudioEntity *m_pEntity[CUSBAudioEntity::MaximumID+1];
};

#endif
