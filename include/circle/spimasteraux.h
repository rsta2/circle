//
// spimasteraux.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2018  R. Stange <rsta2@o2online.de>
//
// Supported features:
//	AUX SPI0 device only (is system global SPI1)
//	Chip select lines (CE0, CE1 or CE2) are active low
//	CPOL, CPHA are fixed
//	Polled operation only
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
#ifndef _circle_spimasteraux_h
#define _circle_spimasteraux_h

#include <circle/gpiopin.h>
#include <circle/spinlock.h>
#include <circle/types.h>

class CSPIMasterAUX		/// Driver for the auxiliary SPI master (SPI1)
{
public:
	CSPIMasterAUX (unsigned nClockSpeed = 500000);
	~CSPIMasterAUX (void);

	boolean Initialize (void);

	// modify default configuration before specific transfer
	// not protected by internal spinlock for multi-core operation
	void SetClock (unsigned nClockSpeed);			// in Hz

	// returns number of read bytes or < 0 on failure
	int Read (unsigned nChipSelect, void *pBuffer, unsigned nCount);

	// returns number of written bytes or < 0 on failure
	int Write (unsigned nChipSelect, const void *pBuffer, unsigned nCount);

	// returns number of bytes transfered or < 0 on failure
	int WriteRead (unsigned nChipSelect,
		       const void *pWriteBuffer, void *pReadBuffer, unsigned nCount);

private:
	unsigned m_nClockSpeed;

	CGPIOPin m_SCLK;
	CGPIOPin m_MOSI;
	CGPIOPin m_MISO;
	CGPIOPin m_CE0;
	CGPIOPin m_CE1;
	CGPIOPin m_CE2;

	unsigned m_nCoreClockRate;
	u32 m_nClockDivider;

	CSpinLock m_SpinLock;
};

#endif
