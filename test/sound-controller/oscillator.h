//
// oscillator.h
//
// General purpose oscillator
//
// MiniSynth Pi - A virtual analogue synthesizer for Raspberry Pi
// Copyright (C) 2017-2022  R. Stange <rsta2@o2online.de>
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
#ifndef _oscillator_h
#define _oscillator_h

#include "synthmodule.h"

enum TWaveform
{
	WaveformSine,
	WaveformSquare,
	WaveformSawtooth,
	WaveformTriangle,
	WaveformPulse12,
	WaveformPulse25,
	WaveformNoise,
	WaveformUnknown
};

class COscillator : public CSynthModule
{
public:
	COscillator (CSynthModule *pModulator = 0);
	~COscillator (void);

	void SetWaveform (TWaveform Waveform);
	void SetFrequency (float fFrequency);			// in Hz
	void SetModulationVolume (float fVolume);		// [0.0, 1.0]

	void NextSample (void);
	float GetOutputLevel (void) const;			// returns [-1.0, 1.0]

private:
	int GetRandom (void);
	static const int RandomMax = 32767;

private:
	CSynthModule *m_pModulator;

	TWaveform m_Waveform;
	float m_fFrequency;
	float m_fModulationVolume;

	unsigned m_nSampleCount;

	float m_fOutputLevel;

	unsigned long m_ulRandomNext;

	static float s_SineTable[];
};

#endif
