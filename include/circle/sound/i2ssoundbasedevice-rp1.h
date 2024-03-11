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

#include <circle/sound/soundbasedevice.h>
#include <circle/sound/soundcontroller.h>
#include <circle/interrupt.h>
#include <circle/i2cmaster.h>
#include <circle/gpiopin.h>
#include <circle/gpioclock.h>
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
			     unsigned	       nSampleRate = 48000,
			     unsigned	       nChunkSize  = 16,	// ignored
			     bool	       bSlave      = FALSE,
			     CI2CMaster       *pI2CMaster  = 0,
			     u8		       ucI2CAddress = 0,
			     TDeviceMode       DeviceMode  = DeviceModeTXOnly);

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

	void InterruptHandler (void);
	static void InterruptStub (void *pParam);

	boolean ControllerFactory (void);

private:
	CInterruptSystem *m_pInterruptSystem;
	unsigned m_nSampleRate;
	//unsigned m_nChunkSize;
	boolean m_bSlave;
	CI2CMaster *m_pI2CMaster;
	u8 m_ucI2CAddress;
	TDeviceMode m_DeviceMode;

	CGPIOPin   m_PCMCLKPin;
	CGPIOPin   m_PCMFSPin;
	CGPIOPin   m_PCMDINPin;
	CGPIOPin   m_PCMDOUTPin;
	CGPIOClock m_Clock;

	uintptr m_ulBase;

	boolean m_bInterruptConnected;
	volatile boolean m_bActive;
	boolean m_bError;

	unsigned m_nFIFOThreshold;

	boolean m_bControllerInited;
	CSoundController *m_pController;
};

#endif
