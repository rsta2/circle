//
// soundcontroller.h
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
#ifndef _circle_sound_soundcontroller_h
#define _circle_sound_soundcontroller_h

#include <circle/types.h>

class CSoundController		/// Optional controller of a sound device
{
public:
	enum TOutputSelector
	{
		OutputSelectorDefault,
		OutputSelectorLine,
		OutputSelectorHeadphones,
		OutputSelectorHDMI,
		OutputSelectorUnknown
	};

	enum TInputSelector
	{
		InputSelectorDefault,
		InputSelectorLine,
		InputSelectorMicrophone,
		InputSelectorUnknown
	};

	struct TRange
	{
		int Min;
		int Max;
	};

public:
	virtual ~CSoundController (void) {}

	virtual boolean Probe (void) { return TRUE; }

	virtual void SelectOutput (TOutputSelector Selector) {}
	virtual void SelectInput (TInputSelector Selector) {}

	virtual void SetOutputVolume (int ndB) {}
	virtual void SetInputVolume (int ndB) {}

	virtual const TRange GetOutputVolumeRange (void) const { return { -100, 20 }; }
	virtual const TRange GetInputVolumeRange (void) const { return { -100, 20 }; }
};

#endif
