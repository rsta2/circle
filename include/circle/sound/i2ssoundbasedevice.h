//
// i2ssoundbasedevice.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2022  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_sound_i2ssoundbasedevice_h
#define _circle_sound_i2ssoundbasedevice_h

#include <circle/sound/soundbasedevice.h>
#include <circle/sound/soundcontroller.h>
#include <circle/interrupt.h>
#include <circle/i2cmaster.h>
#include <circle/gpiopin.h>
#include <circle/gpioclock.h>
#include <circle/sound/dmasoundbuffers.h>
#include <circle/logger.h>
#include <circle/types.h>

class CI2SSoundBaseDevice : public CSoundBaseDevice	/// Low level access to the I2S sound device
{
public:
	enum TDeviceMode
	{
		DeviceModeTXOnly,	///< I2S output
		DeviceModeRXOnly,	///< I2S input
		DeviceModeTXRX,		///< I2S output and input
		DeviceModeUnknown
	};

public:
	/// \param pInterrupt	pointer to the interrupt system object
	/// \param nSampleRate	sample rate in Hz
	/// \param nChunkSize	twice the number of samples (words) to be handled\n
	///			with one call to GetChunk() (one word per stereo channel)
	/// \param bSlave	enable slave mode (PCM clock and FS clock are inputs)
	/// \param pI2CMaster	pointer to the I2C master object (0 if no I2C DAC init required)
	/// \param ucI2CAddress I2C slave address of the DAC (0 for auto probing 0x4C and 0x4D)
	/// \param DeviceMode	which transfer direction to use?
	CI2SSoundBaseDevice (CInterruptSystem *pInterrupt,
			     unsigned	       nSampleRate = 192000,
			     unsigned	       nChunkSize  = 8192,
			     bool	       bSlave      = FALSE,
			     CI2CMaster       *pI2CMaster  = 0,
			     u8		       ucI2CAddress = 0,
			     TDeviceMode       DeviceMode  = DeviceModeTXOnly);

	virtual ~CI2SSoundBaseDevice (void);

	/// \return Minium value of one sample
	int GetRangeMin (void) const override;
	/// \return Maximum value of one sample
	int GetRangeMax (void) const override;

	/// \brief Starts the I2S and DMA operation
	boolean Start (void) override;

	/// \brief Cancels the I2S and DMA operation
	/// \note Cancel takes effect after a short delay
	void Cancel (void) override;

	/// \return Is I2S and DMA operation running?
	boolean IsActive (void) const override;

	/// \return Pointer to sound controller object or nullptr, if not supported.
	CSoundController *GetController (void) override;

protected:
	/// \brief May overload this to provide the sound samples!
	/// \param pBuffer	buffer where the samples have to be placed
	/// \param nChunkSize	size of the buffer in words (same as given to constructor)
	/// \return Number of words written to the buffer (normally nChunkSize),\n
	///	    Transfer will stop if 0 is returned
	/// \note Each sample consists of two words (Left channel, right channel)\n
	///	  Each word must be between GetRangeMin() and GetRangeMax()
	/// virtual unsigned GetChunk (u32 *pBuffer, unsigned nChunkSize);

	/// \brief Overload this to consume the received sound samples
	/// \param pBuffer    Buffer where the samples have been placed
	/// \param nChunkSize Size of the buffer in words
	/// \note Each sample consists of two words (Left channel, right channel)
	/// virtual void PutChunk (const u32 *pBuffer, unsigned nChunkSize);

private:
	void RunI2S (void);
	void StopI2S (void);

	static unsigned TXCompletedHandler (boolean bStatus, u32 *pBuffer,
					    unsigned nChunkSize, void *pParam);
	static unsigned RXCompletedHandler (boolean bStatus, u32 *pBuffer,
					    unsigned nChunkSize, void *pParam);

	boolean ControllerFactory (void);

private:
	unsigned m_nChunkSize;
	bool     m_bSlave;
	CI2CMaster *m_pI2CMaster;
	u8 m_ucI2CAddress;
	TDeviceMode m_DeviceMode;

	CGPIOPin   m_PCMCLKPin;
	CGPIOPin   m_PCMFSPin;
	CGPIOPin   m_PCMDINPin;
	CGPIOPin   m_PCMDOUTPin;
	CGPIOClock m_Clock;

	volatile boolean m_bError;

	CDMASoundBuffers m_TXBuffers;
	CDMASoundBuffers m_RXBuffers;

	boolean m_bControllerInited;
	CSoundController *m_pController;
};

#endif
