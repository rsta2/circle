//
// spimaster-rp1.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2024  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_spimaster_rp1_h
#define _circle_spimaster_rp1_h

#include <circle/gpiopin.h>
#include <circle/genericlock.h>
#include <circle/types.h>

#ifndef _circle_spimaster_h
	#error Do not include this header file directly!
#endif

class CSPIMaster
{
public:
	const unsigned ChipSelectNone = 4;

public:
	CSPIMaster (unsigned nClockSpeed = 500000, unsigned CPOL = 0, unsigned CPHA = 0,
		    unsigned nDevice = 0);

	~CSPIMaster (void);

	boolean Initialize (void);

	void SetClock (unsigned nClockSpeed);
	void SetMode (unsigned CPOL, unsigned CPHA);

	void SetCSHoldTime (unsigned nMicroSeconds);	// ignored

	int Read (unsigned nChipSelect, void *pBuffer, unsigned nCount);

	int Write (unsigned nChipSelect, const void *pBuffer, unsigned nCount);

	int WriteRead (unsigned nChipSelect, const void *pWriteBuffer, void *pReadBuffer,
		       unsigned nCount);

private:
	uintptr  m_ulBaseAddress;
	boolean  m_bValid;

	CGPIOPin m_SCLK;
	CGPIOPin m_MOSI;
	CGPIOPin m_MISO;
	CGPIOPin m_CE0;
	CGPIOPin m_CE1;

	CGenericLock m_Lock;
};

#endif
