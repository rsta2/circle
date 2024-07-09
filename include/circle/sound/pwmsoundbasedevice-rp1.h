//
// pwmsoundbasedevice-rp1.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2024  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_sound_pwmsoundbasedevice_rp1_h
#define _circle_sound_pwmsoundbasedevice_rp1_h

#ifndef _circle_sound_pwmsoundbasedevice_h
	#error Do not include this header file directly!
#endif

#include <circle/sound/soundbasedevice.h>
#include <circle/interrupt.h>
#include <circle/gpiopin.h>
#include <circle/gpioclock.h>
#include <circle/dmachannel-rp1.h>
#include <circle/spinlock.h>
#include <circle/types.h>

class CPWMSoundBaseDevice : public CSoundBaseDevice
{
public:
	CPWMSoundBaseDevice (CInterruptSystem *pInterrupt,
			     unsigned	       nSampleRate = 44100,
			     unsigned	       nChunkSize  = 2048);

	virtual ~CPWMSoundBaseDevice (void);

	int GetRangeMin (void) const override;
	int GetRangeMax (void) const override;
	unsigned GetRange (void) const { return (unsigned) GetRangeMax (); }

	boolean Start (void) override;

	void Cancel (void) override;

	boolean IsActive (void) const override;

private:
	boolean RunPWM (void);
	void StopPWM (void);

	void DMACompletionRoutine (unsigned nChannel, unsigned nBuffer, boolean bStatus);
	static void DMACompletionStub (unsigned nChannel, unsigned nBuffer,
				       boolean bStatus, void *pParam);

private:
	enum TState
	{
		StateIdle,
		StateRunning,
		StateCanceled,
		StateFailed,
		StateUnknown
	};

private:
	unsigned m_nChunkSize;
	unsigned m_nRange;

	unsigned   m_nChannel1;
	CGPIOPin   m_Audio1;
	unsigned   m_nChannel2;
	CGPIOPin   m_Audio2;
	CGPIOClock m_Clock;

	volatile TState m_State;

	CDMAChannelRP1 m_DMAChannel;
	u32 *m_pDMABuffer[2];

	CSpinLock m_SpinLock;
};

#endif
