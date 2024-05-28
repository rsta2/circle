//
// i2ssoundbasedevice-rp1.h
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
#ifndef _circle_sound_i2ssoundbasedevice_rp1_h
#define _circle_sound_i2ssoundbasedevice_rp1_h

#ifndef _circle_sound_i2ssoundbasedevice_h
	#error Do not include this header file directly!
#endif

//#define USE_I2S_SOUND_IRQ

#include <circle/sound/soundbasedevice.h>
#include <circle/sound/soundcontroller.h>
#include <circle/interrupt.h>
#include <circle/i2cmaster.h>
#include <circle/gpiopin.h>
#include <circle/gpioclock.h>
#include <circle/dmachannel-rp1.h>
#include <circle/spinlock.h>
#include <circle/types.h>

class CI2SSoundBaseDevice : public CSoundBaseDevice
{
public:
	enum TDeviceMode
	{
		DeviceModeTXOnly,
		DeviceModeRXOnly,
		DeviceModeTXRX,		// not supported
		DeviceModeUnknown
	};

public:
	CI2SSoundBaseDevice (CInterruptSystem *pInterrupt,
			     unsigned	       nSampleRate = 192000,
			     unsigned	       nChunkSize  = 8192,	// ignored in IRQ mode
			     bool	       bSlave      = FALSE,
			     CI2CMaster       *pI2CMaster  = 0,
			     u8		       ucI2CAddress = 0,
			     TDeviceMode       DeviceMode  = DeviceModeTXOnly,
			     unsigned	       nHWChannels = 2);	// 2 or 8

	virtual ~CI2SSoundBaseDevice (void);

	int GetRangeMin (void) const override;
	int GetRangeMax (void) const override;

	boolean Start (void) override;

	void Cancel (void) override;

	boolean IsActive (void) const override;

	CSoundController *GetController (void) override;

private:
	boolean RunI2S (void);
	void StopI2S (void);

#ifdef USE_I2S_SOUND_IRQ
	void InterruptHandler (void);
	static void InterruptStub (void *pParam);
#else
	void DMACompletionRoutine (unsigned nChannel, unsigned nBuffer, boolean bStatus);
	static void DMACompletionStub (unsigned nChannel, unsigned nBuffer,
				       boolean bStatus, void *pParam);
#endif

	boolean ControllerFactory (void);

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
#ifdef USE_I2S_SOUND_IRQ
	CInterruptSystem *m_pInterruptSystem;
#else
	unsigned m_nChunkSize;
#endif
	unsigned m_nSampleRate;
	boolean m_bSlave;
	CI2CMaster *m_pI2CMaster;
	u8 m_ucI2CAddress;
	TDeviceMode m_DeviceMode;
	unsigned m_nHWChannels;

	CGPIOPin   m_PCMCLKPin;
	CGPIOPin   m_PCMFSPin;
	CGPIOPin   m_PCMDINPin;
	CGPIOPin   m_PCMDOUTPin;
	CGPIOPin   m_PCMDOUTPin1;
	CGPIOPin   m_PCMDOUTPin2;
	CGPIOPin   m_PCMDOUTPin3;
	CGPIOClock m_Clock;

	uintptr m_ulBase;

#ifdef USE_I2S_SOUND_IRQ
	boolean m_bInterruptConnected;
#endif

	volatile TState m_State;

	unsigned m_nFIFOThreshold;

#ifndef USE_I2S_SOUND_IRQ
	CDMAChannelRP1 m_DMAChannel;
	u32 *m_pDMABuffer[2];
#endif

	boolean m_bControllerInited;
	CSoundController *m_pController;

#ifndef USE_I2S_SOUND_IRQ
	CSpinLock m_SpinLock;
#endif
};

#endif
