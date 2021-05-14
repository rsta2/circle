//
// i2ssoundbasedevice.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_i2ssoundbasedevice_h
#define _circle_i2ssoundbasedevice_h

#include <circle/soundbasedevice.h>
#include <circle/interrupt.h>
#include <circle/i2cmaster.h>
#include <circle/gpiopin.h>
#include <circle/gpioclock.h>
#include <circle/dmachannel.h>
#include <circle/spinlock.h>
#include <circle/types.h>

enum TI2SSoundState
{
	I2SSoundIdle,
	I2SSoundRunning,
	I2SSoundCancelled,
	I2SSoundTerminating,
	I2SSoundError,
	I2SSoundUnknown
};

class CI2SSoundBaseDevice : public CSoundBaseDevice	/// Low level access to the I2S sound device
{
public:
	/// \param pInterrupt	pointer to the interrupt system object
	/// \param nSampleRate	sample rate in Hz
	/// \param nChunkSize	twice the number of samples (words) to be handled\n
	///			with one call to GetChunk() (one word per stereo channel)
	/// \param bSlave	enable slave mode (PCM clock and FS clock are inputs)
	/// \param pI2CMaster	pointer to the I2C master object (0 if no I2C DAC init required)
	/// \param ucI2CAddress I2C slave address of the DAC (0 for auto probing 0x4C and 0x4D)
	CI2SSoundBaseDevice (CInterruptSystem *pInterrupt,
			     unsigned	       nSampleRate = 192000,
			     unsigned	       nChunkSize  = 8192,
			     bool	       bSlave      = FALSE,
			     CI2CMaster       *pI2CMaster  = 0,
			     u8		       ucI2CAddress = 0);

	virtual ~CI2SSoundBaseDevice (void);

	/// \return Minium value of one sample
	int GetRangeMin (void) const;
	/// \return Maximum value of one sample
	int GetRangeMax (void) const;

	/// \brief Starts the I2S and DMA operation
	boolean Start (void);

	/// \brief Cancels the I2S and DMA operation
	/// \note Cancel takes effect after a short delay
	void Cancel (void);

	/// \return Is I2S and DMA operation running?
	boolean IsActive (void) const;

protected:
	/// \brief May overload this to provide the sound samples!
	/// \param pBuffer	buffer where the samples have to be placed
	/// \param nChunkSize	size of the buffer in words (same as given to constructor)
	/// \return Number of words written to the buffer (normally nChunkSize),\n
	///	    Transfer will stop if 0 is returned
	/// \note Each sample consists of two words (Left channel, right channel)\n
	///	  Each word must be between GetRangeMin() and GetRangeMax()
	/// virtual unsigned GetChunk (u32 *pBuffer, unsigned nChunkSize);

private:
	boolean GetNextChunk (boolean bFirstCall = FALSE);

	void RunI2S (void);
	void StopI2S (void);

	void InterruptHandler (void);
	static void InterruptStub (void *pParam);

	void SetupDMAControlBlock (unsigned nID);

	boolean InitPCM51xx (u8 ucI2CAddress);

private:
	CInterruptSystem *m_pInterruptSystem;
	unsigned m_nChunkSize;
	bool     m_bSlave;
	CI2CMaster *m_pI2CMaster;
	u8 m_ucI2CAddress;

	CGPIOPin   m_PCMCLKPin;
	CGPIOPin   m_PCMFSPin;
	CGPIOPin   m_PCMDOUTPin;
	CGPIOClock m_Clock;

	boolean m_bI2CInited;
	boolean m_bIRQConnected;
	volatile TI2SSoundState m_State;

	unsigned m_nDMAChannel;
	u32 *m_pDMABuffer[2];
	u8 *m_pControlBlockBuffer[2];
	TDMAControlBlock *m_pControlBlock[2];

	unsigned m_nNextBuffer;			// 0 or 1

	CSpinLock m_SpinLock;
};

#endif
