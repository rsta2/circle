//
// hdmisoundbasedevice.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2021  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_hdmisoundbasedevice_h
#define _circle_hdmisoundbasedevice_h

#include <circle/soundbasedevice.h>
#include <circle/interrupt.h>
#include <circle/gpioclock.h>
#include <circle/dmachannel.h>
#include <circle/spinlock.h>
#include <circle/types.h>

enum THDMISoundState
{
	HDMISoundCreated,
	HDMISoundIdle,
	HDMISoundRunning,
	HDMISoundCancelled,
	HDMISoundTerminating,
	HDMISoundError,
	HDMISoundUnknown
};

/// \note This driver does not support HDMI1 on Raspberry Pi 4 (HDMI0 only).

class CHDMISoundBaseDevice : public CSoundBaseDevice	/// Low level access to the HDMI sound device
{
public:
	/// \param pInterrupt	pointer to the interrupt system object
	/// \param nSampleRate	sample rate in Hz
	/// \param nChunkSize	twice the number of samples (words) to be handled\n
	///			with one call to GetChunk() (one word per stereo channel),\n
	///			must be a multiple of 384
	CHDMISoundBaseDevice (CInterruptSystem *pInterrupt,
			      unsigned	        nSampleRate = 48000,
			      unsigned	        nChunkSize  = 384 * 10);

	virtual ~CHDMISoundBaseDevice (void);

	/// \brief Starts the HDMI sound and DMA operation
	boolean Start (void);

	/// \brief Cancels the HDMI sound and DMA operation
	/// \note Cancel takes effect after a short delay
	void Cancel (void);

	/// \return Is HDMI sound and DMA operation running?
	boolean IsActive (void) const;

protected:
	/// \brief May overload this to provide the sound samples!
	/// \param pBuffer	buffer where the samples have to be placed
	/// \param nChunkSize	size of the buffer in words (same as given to constructor)
	/// \return Number of words written to the buffer (normally nChunkSize),\n
	///	    Transfer will stop if 0 is returned
	/// \note nChunkSize is a multiple of 384 samples (sub-frames).\n
	///	  Each frame consists of two sub-frames (left/right samples).\n
	///	  Each sample has to be converted using ConvertIEC958Sample()\n
	///	  to apply the IEC958 framing, before writing it into the buffer.
	/// virtual unsigned GetChunk (u32 *pBuffer, unsigned nChunkSize);

private:
	boolean GetNextChunk (void);

	void RunHDMI (void);
	void StopHDMI (void);

	void InterruptHandler (void);
	static void InterruptStub (void *pParam);

	void SetupDMAControlBlock (unsigned nID);

	void ResetHDMI (void);
	boolean SetAudioInfoFrame (void);
	static u32 SampleRateToHWFormat (unsigned nSampleRate);

	static boolean WaitForBit (uintptr nIOAddress, u32 nMask, boolean bWaitUntilSet,
				   unsigned nMsTimeout);

#if RASPPI <= 3
	unsigned long GetHSMClockRate (void);
	unsigned long GetPixelClockRate (void);
#endif

	static void rational_best_approximation (
		unsigned long given_numerator, unsigned long given_denominator,
		unsigned long max_numerator, unsigned long max_denominator,
		unsigned long *best_numerator, unsigned long *best_denominator);

private:
	CInterruptSystem *m_pInterruptSystem;
	unsigned m_nSampleRate;
	unsigned m_nChunkSize;

	unsigned long m_ulAudioClockRate;
	unsigned long m_ulPixelClockRate;

	boolean m_bIRQConnected;
	volatile THDMISoundState m_State;

	unsigned m_nDMAChannel;
	u32 *m_pDMABuffer[2];
	u8 *m_pControlBlockBuffer[2];
	TDMAControlBlock *m_pControlBlock[2];

	unsigned m_nNextBuffer;			// 0 or 1

	CSpinLock m_SpinLock;
};

#endif
