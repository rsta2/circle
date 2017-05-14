//
// spidio.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016  R. Stange <rsta2@o2online.de>
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
#ifndef _dio_spidio_h
#define _dio_spidio_h

#include <circle/spimaster.h>
#include <circle/types.h>

#define DIO_FUNC_IO0		0		// functions
#define DIO_FUNC_IO1		1
#define DIO_FUNC_IO2		2
#define DIO_FUNC_IO3		3
#define DIO_FUNC_IO4		4
#define DIO_FUNC_IO5		5
#define DIO_FUNC_IO6		6
#define DIO_FUNC_MAX		DIO_FUNC_IO6

#define DIO_INPUT		0		// modes
#define DIO_OUTPUT		1

#define DIO_LOW			0		// levels
#define DIO_HIGH		1

#define DIO_MASK(func, val)	((val) << (func))

// analog modes
#define DIO_ANALOG_IO0		0x07		// single input
#define DIO_ANALOG_IO1		0x03
#define DIO_ANALOG_IO3		0x02
#define DIO_ANALOG_IO4		0x01
#define DIO_ANALOG_IO6		0x00

#define DIO_ANALOG_IO0_IO1	0x38		// differential measurement
#define DIO_ANALOG_IO1_IO0	0x18
#define DIO_ANALOG_IO1_IO3	0x30
#define DIO_ANALOG_IO1_IO4	0x2E
#define DIO_ANALOG_IO1_IO6	0x2A
#define DIO_ANALOG_IO3_IO1	0x10
#define DIO_ANALOG_IO3_IO4	0x2C
#define DIO_ANALOG_IO4_IO1	0x0E
#define DIO_ANALOG_IO4_IO3	0x0C
#define DIO_ANALOG_IO4_IO6	0x28
#define DIO_ANALOG_IO6_IO1	0x0A
#define DIO_ANALOG_IO6_IO4	0x08

class CSPIDIODevice
{
public:
	CSPIDIODevice (CSPIMaster *pSPIMaster, unsigned nChipSelect, u8 ucAddress = 0x84);
	~CSPIDIODevice (void);

	boolean Initialize (void);

	unsigned GetVersion (void) const;	// returns 10 up

	// digital output/input
	boolean SetModes (u8 ucModeMask);

	boolean SetAllOutputs (u8 ucMask);
	boolean SetOutput (unsigned nFunction, unsigned nLevel);

	boolean GetAllInputs (u8 *pMask);
	boolean GetInput (unsigned nFunction, unsigned *pLevel);

	// analog input (on dio v1.2 and up only)
	boolean SetAnalogChannels (unsigned nChannels);			// number of channels to use
#define DIO_ANALOG_CHANNELS	7

	boolean CoupleAnalogChannel (unsigned nChannel,			// 0..6
				     unsigned nAnalogMode,		// DIO_ANALOG_*
				     boolean  bReference_1_1V = FALSE);	// default: power supply voltage

	// may add multiple samples and shift the result down, to increase resolution
	// See: http://www.bitwizard.nl/wiki/index.php/Analog_inputs#Adding_and_averaging
	boolean SetAnalogSamples (unsigned nSamples,			// should be a power of 2
				  unsigned nBitShift);

	boolean GetAnalogValueRaw (unsigned nChannel, unsigned *pValue);
#define DIO_ANALOG_VALUE_MAX	1023

	boolean GetAnalogValue (unsigned nChannel, unsigned *pValue);	// added and shifted value

private:
	static unsigned ConvertVersion (const char *pString);

private:
	CSPIMaster *m_pSPIMaster;
	unsigned    m_nChipSelect;
	u8	    m_ucAddress;
	unsigned    m_nVersion;
};

#endif
