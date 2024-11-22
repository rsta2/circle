//
// xpt2046touchscreen.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2024  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_input_xpt2046touchscreen_h
#define _circle_input_xpt2046touchscreen_h

#include <circle/input/touchscreen.h>
#include <circle/spimaster.h>
#include <circle/gpiomanager.h>
#include <circle/gpiopin.h>
#include <circle/types.h>

class CXPT2046TouchScreen	/// Driver for XPT2046-based touch screens
{
public:
	/// \param pSPIMaster Pointer to SPI master object
	/// \param nChipSelect Chip select to use (CS#)
	/// \param pGPIOManager Pointer to GPIO manager object
	/// \param nIRQPin SoC GPIO pin number, where T_IRQ pin is connected
	CXPT2046TouchScreen (CSPIMaster *pSPIMaster, unsigned nChipSelect,
			     CGPIOManager *pGPIOManager, unsigned nIRQPin);

	~CXPT2046TouchScreen (void);

	/// \param nDegrees Counterclockwise rotation in degrees (0, 90, 180, 270)
	void SetRotation (unsigned nDegrees);
	/// \return Counterclockwise rotation in degrees (0, 90, 180, 270)
	unsigned GetRotation (void)		{ return m_nRotation; }

	/// \return Operation successful?
	boolean Initialize (void);

private:
	void Update (void);
	static void UpdateStub (void *pParam);

	static void GPIOInterruptHandler (void *pParam);

	static s16 BestTwoAvg (s16 x, s16 y, s16 z);

private:
	static const unsigned SPIClockSpeed = 2000000;		// Hz

	static const unsigned ThresholdMicros = 15000;

	static const s16 Z_Threshold_Active = 75;
	static const s16 Z_Threshold_Pressed = 300;

private:
	CSPIMaster *m_pSPIMaster;
	unsigned m_nChipSelect;

	CGPIOPin m_IRQPin;
	boolean m_bInterruptConnected;

	unsigned m_nRotation;

	CTouchScreenDevice *m_pDevice;

	volatile boolean m_bActive;
	unsigned m_nLastTicks;
	boolean m_bFingerDown;
};

#endif
