//
// pwmsoundbasedevice.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2017  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_pwmsoundbasedevice_h
#define _circle_pwmsoundbasedevice_h

#include <circle/interrupt.h>
#include <circle/gpiopin.h>
#include <circle/gpioclock.h>
#include <circle/dmachannel.h>
#include <circle/spinlock.h>
#include <circle/types.h>

enum TPWMSoundState
{
	PWMSoundIdle,
	PWMSoundRunning,
	PWMSoundCancelled,
	PWMSoundTerminating,
	PWMSoundError,
	PWMSoundUnknown
};

class CPWMSoundBaseDevice	/// Low level access to the PWM sound device (on 3.5mm headphone jack)
{
public:
	/// \param pInterrupt	pointer to the interrupt system object
	/// \param nSampleRate	sample rate in Hz
	/// \param nChunkSize	twice the number of samples (words) to be handled\n
	///			with one call to GetChunk() (one word per stereo channel)
	CPWMSoundBaseDevice (CInterruptSystem *pInterrupt,
			     unsigned	       nSampleRate = 44100,
			     unsigned	       nChunkSize  = 2048);

	virtual ~CPWMSoundBaseDevice (void);

	/// \return PWM range available for one sample
	unsigned GetRange (void) const;

	/// \brief Starts the PWM and DMA operation
	void Start (void);

	/// \brief Cancels the PWM and DMA operation
	/// \note Cancel takes effect after a short delay
	void Cancel (void);

	/// \return Is PWM and DMA operation running?
	boolean IsActive (void) const;

	/// \brief Overload this to provide the sound samples!
	/// \param pBuffer	buffer where the samples have to be placed
	/// \param nChunkSize	size of the buffer in words (same as given to constructor)
	/// \return Number of words written to the buffer (normally nChunkSize),\n
	///	    Transfer will stop if 0 is returned
	/// \note Each sample consists of two words (Left channel, right channel)\n
	///	  Each word must be between 0 and the value returned by GetRange()
	virtual unsigned GetChunk (u32 *pBuffer, unsigned nChunkSize) = 0;

private:
	boolean GetNextChunk (void);

	void RunPWM (void);
	void StopPWM (void);

	void InterruptHandler (void);
	static void InterruptStub (void *pParam);

	void SetupDMAControlBlock (unsigned nID);

private:
	CInterruptSystem *m_pInterruptSystem;
	unsigned m_nChunkSize;
	unsigned m_nRange;

	CGPIOPin   m_Audio1;
	CGPIOPin   m_Audio2;
	CGPIOClock m_Clock;

	boolean m_bIRQConnected;
	volatile TPWMSoundState m_State;

	u32 *m_pDMABuffer[2];
	u8 *m_pControlBlockBuffer[2];
	TDMAControlBlock *m_pControlBlock[2];

	unsigned m_nNextBuffer;			// 0 or 1

	CSpinLock m_SpinLock;
};

#endif
