//
// soundcontroller.h
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
#ifndef _circle_sound_soundcontroller_h
#define _circle_sound_soundcontroller_h

#include <circle/macros.h>
#include <circle/types.h>

/// \note The methods of the sound controller are callable from TASK_LEVEL only.

class CSoundController		/// Optional controller of a sound device
{
public:
	enum TProperty
	{
		PropertyDirectionSupported	= BIT(0),	///< Output/input supported?
		PropertyMultiJackOperation	= BIT(1)	///< Enable multiple jacks at once?
	};

	enum TJack
	{
		JackUnknown	= 0,

		JackDefaultOut	= BIT(0),	///< default or currently active output
		JackLineOut	= BIT(1),
		JackSpeaker	= BIT(2),
		JackHeadphone	= BIT(3),
		JackHDMI	= BIT(4),
		JackSPDIFOut	= BIT(5),

		JackDefaultIn	= BIT(8),	///< default or currently active input
		JackLineIn	= BIT(9),
		JackMicrophone	= BIT(10)
	};

	boolean IsOutputJack (TJack Jack) const { return !!(Jack & 0x00FF); }
	boolean IsInputJack (TJack Jack) const { return !!(Jack & 0xFF00); }

	enum TChannel
	{
		ChannelAll,
		Channel1,
		ChannelLeft	= Channel1,
		Channel2,
		ChannelRight	= Channel2,
		Channel3,
		Channel4,
		Channel5,
		Channel6,
		Channel7,
		Channel8,
		Channel9,
		Channel10,
		Channel11,
		Channel12,
		Channel13,
		Channel14,
		Channel15,
		Channel16,
		Channel17,
		Channel18,
		Channel19,
		Channel20,
		Channel21,
		Channel22,
		Channel23,
		Channel24,
		Channel25,
		Channel26,
		Channel27,
		Channel28,
		Channel29,
		Channel30,
		Channel31,
		Channel32,
		ChannelUnknown
	};

	enum TControl
	{
		ControlMute,			///< Mute value is 0 (disable) or 1 (enable)
		ControlVolume,			///< Volume value is in dB
		ControlUnknown
	};

	struct TControlInfo
	{
		boolean	Supported;
		int	RangeMin;
		int	RangeMax;
	};

public:
	virtual ~CSoundController (void) {}

	/// \brief Used internally to initialize sound controller
	virtual boolean Probe (void) { return TRUE; }

	/// \return Bitmask with TProperty values or'ed together
	virtual u32 GetOutputProperties (void) const { return 0; }
	/// \return Bitmask with TProperty values or'ed together
	virtual u32 GetInputProperties (void) const { return 0; }

	/// \param Jack Jack to be enabled
	/// \return Operation successful?
	/// \note Can be called multiple times on different jacks,
	///	  if PropertyMultiJackOperation is available.
	/// \note Automatically disables the previous active jack,
	///	  if PropertyMultiJackOperation is not available.
	virtual boolean EnableJack (TJack Jack) { return FALSE; }
	/// \param Jack Jack to be disabled
	/// \return Operation successful?
	/// \note Allways fails without PropertyMultiJackOperation available.
	virtual boolean DisableJack (TJack Jack) { return FALSE; }

	/// \param Control Control selector
	/// \param Jack Affected jack
	/// \param Channel Affected channel(s)
	/// \return Info about the control in the selected configuration
	/// \note A control may be supported for ChannelAll, but not for ChannelLeft/Right.
	virtual const TControlInfo GetControlInfo (TControl Control, TJack Jack,
						   TChannel Channel) const { return { FALSE, 0, 0 }; }
	/// \param Control Control selector
	/// \param Jack Affected jack
	/// \param Channel Affected channel(s)
	/// \param nValue Value to be set
	/// \return Operation successful?
	virtual boolean SetControl (TControl Control, TJack Jack, TChannel Channel, int nValue)
									{ return FALSE; }
};

#endif
