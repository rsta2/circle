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

class CSPIDIODevice
{
public:
	CSPIDIODevice (CSPIMaster *pSPIMaster, unsigned nChipSelect, u8 ucAddress = 0x84);
	~CSPIDIODevice (void);

	boolean Initialize (void);

	unsigned GetVersion (void) const;	// returns 10 up

	boolean SetModes (u8 ucModeMask);

	boolean SetAllOutputs (u8 ucMask);
	boolean SetOutput (unsigned nFunction, unsigned nLevel);

	boolean GetAllInputs (u8 *pMask);
	boolean GetInput (unsigned nFunction, unsigned *pLevel);

private:
	static unsigned ConvertVersion (const char *pString);

private:
	CSPIMaster *m_pSPIMaster;
	unsigned    m_nChipSelect;
	u8	    m_ucAddress;
	unsigned    m_nVersion;
};

#endif
