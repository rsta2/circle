//
// pwmsounddevice.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_pwmsounddevice_h
#define _circle_pwmsounddevice_h

#include <circle/interrupt.h>
#include <circle/gpiopin.h>
#include <circle/gpioclock.h>
#include <circle/dmachannel.h>
#include <circle/types.h>

class CPWMSoundDevice
{
public:
	CPWMSoundDevice (CInterruptSystem *pInterrupt);
	~CPWMSoundDevice (void);

	void Playback (void	*pSoundData,		// sample rate 44100 Hz
		       unsigned  nSamples,		// for Stereo the L/R samples are count as one
		       unsigned  nChannels,		// 1 (Mono) or 2 (Stereo)
		       unsigned  nBitsPerSample);	// 8 (unsigned sound data) or 16 (signed sound data)

	boolean PlaybackActive (void) const;

	void CancelPlayback (void);			// will be canceled with a short delay

private:
	void Run (void);
	void Stop (void);

	// transfer next chunk into DMA buffer
	unsigned ConvertSample (void);		// returns DMA transfer size in bytes

	void DMACompletionRoutine (boolean bStatus);
	static void DMACompletionStub (unsigned nChannel, boolean bStatus, void *pParam);

private:
	CGPIOPin    m_Audio1;
	CGPIOPin    m_Audio2;
	CGPIOClock  m_Clock;
	CDMAChannel m_DMAChannel;

	u32 *m_pDMABuffer;

	u8	 *m_pSoundData;
	unsigned  m_nSamples;
	unsigned  m_nChannels;
	unsigned  m_nBitsPerSample;

	volatile boolean m_bActive;
};

#endif
