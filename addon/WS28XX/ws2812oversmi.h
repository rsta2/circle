//
// \file ws2812oversmi.h
//
// Driver for multiple WS2812 controlled LED strips
// Original development by Sebastien Nicolas <seba1978@gmx.de>
// Adapted from https://iosoft.blog/2020/09/29/raspberry-pi-multi-channel-ws2812/
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2022  R. Stange <rsta2@o2online.de>
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
#ifndef _ws28xx_ws2812oversmi_h
#define _ws28xx_ws2812oversmi_h

#include <circle/types.h>
#include <circle/smimaster.h>

#define LED_NCHANS		16  // Number of LED channels (8 or 16) - has to be 16 if we're using SD8 or above
#define LED_NBITS		24  // Number of data bits per LED
#define LED_PREBITS		4   // Number of zero bits before LED data
#define LED_POSTBITS	4   // Number of zero bits after LED data
#define BIT_NPULSES		3   // Number of O/P pulses per LED bit


// Length of data for 1 row (1 LED on each channel)
#define LED_DLEN		(LED_NBITS * BIT_NPULSES)

// Offset into Tx data buffer, given LED number in chan
#define LED_TX_OSET(n)	(LED_PREBITS + (LED_DLEN * (n)))
#define TX_BUFF_LEN(n)	(LED_TX_OSET(n) + LED_POSTBITS)

// Transmit data type, 8 or 16 bits
#if LED_NCHANS > 8
#define TXDATA_T		u16
#else
#define TXDATA_T		u8
#endif

// SMI params for a 400 ns cycle time
// NEOPIXEL_SMI_NS is in nanoseconds: even numbers, 2 to 30
#define NEOPIXEL_SMI_WIDTH		(LED_NCHANS > 8 ? SMI16Bits : SMI8Bits)
#define NEOPIXEL_SMI_PACE		0
#if RASPI > 3	// Timings for RPi v4 (1.5 GHz)
#define NEOPIXEL_SMI_NS			10
#define NEOPIXEL_SMI_SETUP		15
#define NEOPIXEL_SMI_STROBE		30
#define NEOPIXEL_SMI_HOLD		15
#else	// Timings for RPi v0-3 (1 GHz)
#define NEOPIXEL_SMI_NS			10
#define NEOPIXEL_SMI_SETUP		10
#define NEOPIXEL_SMI_STROBE		20
#define NEOPIXEL_SMI_HOLD		10
#endif


class CWS2812OverSMI {
public:
	// nSDLinesMask may be for example (1 << 0) | (1 << 5) for 2 LED strips on SD0 (GPIO8) and SD5 (GPIO13)
	CWS2812OverSMI(unsigned nSDLinesMask, unsigned nNumberOfLEDsPerStrip);

	~CWS2812OverSMI();

	unsigned GetLEDCount() const;

	void Update();

	// Accordingly to the constructor, nSDLine may be for example 0 for the first strip on SD0 (GPIO8) or 5 for the second strip on SD5 (GPIO13)
	void SetLED(unsigned nSDLine, unsigned nLEDIndexInStrip, u8 nRed, u8 nGreen, u8 nBlue);

private:
	CSMIMaster m_SMIMaster;
	unsigned m_nLEDCount;
	boolean m_bDirty;
	TXDATA_T *m_pBuffer;
	boolean m_bOnlyOneLine;
};

#endif
