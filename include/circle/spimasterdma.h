//
// spimasterdma.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2022  R. Stange <rsta2@o2online.de>
//
// Supported features:
//	SPI0 device only
//	Standard mode (3-wire) only
//	Chip select lines (CE0, CE1) are active low
//	DMA or polled operation
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
#ifndef _circle_spimasterdma_h
#define _circle_spimasterdma_h

#include <circle/interrupt.h>
#include <circle/dmachannel.h>
#include <circle/gpiopin.h>
#include <circle/types.h>

typedef void TSPICompletionRoutine (boolean bStatus, void *pParam);

class CSPIMasterDMA
{
public:
	const unsigned ChipSelectNone = 3;

public:
	// set bDMAChannelLite to FALSE for very high speeds or transfer sizes >= 64K
	CSPIMasterDMA (CInterruptSystem *pInterruptSystem,
		       unsigned nClockSpeed = 500000, unsigned CPOL = 0, unsigned CPHA = 0,
		       boolean bDMAChannelLite = TRUE);
	~CSPIMasterDMA (void);

	boolean Initialize (void);

	// modify default configuration before specific transfer
	void SetClock (unsigned nClockSpeed);			// in Hz
	void SetMode (unsigned CPOL, unsigned CPHA);

	void SetCompletionRoutine (TSPICompletionRoutine *pRoutine, void *pParam);

	// buffers must be 4-byte aligned
	void StartWriteRead (unsigned nChipSelect, const void *pWriteBuffer, void *pReadBuffer, unsigned nCount);

	// Synchronous (polled) operation for small amounts of data
	// returns number of bytes transferred or < 0 on failure
	int WriteReadSync (unsigned nChipSelect, const void *pWriteBuffer, void *pReadBuffer, unsigned nCount);

private:
	void DMACompletionRoutine (boolean bStatus);
	static void DMACompletionStub (unsigned nChannel, boolean bStatus, void *pParam);

private:
	unsigned m_nClockSpeed;
	unsigned m_CPOL;
	unsigned m_CPHA;

	CDMAChannel m_TxDMA;
	CDMAChannel m_RxDMA;

	CGPIOPin m_SCLK;
	CGPIOPin m_MOSI;
	CGPIOPin m_MISO;
	CGPIOPin m_CE0;
	CGPIOPin m_CE1;

	unsigned m_nCoreClockRate;

	TSPICompletionRoutine *m_pCompletionRoutine;
	void *m_pCompletionParam;
};

#endif
