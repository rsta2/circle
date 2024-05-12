//
// spimasterdma-rp1.h
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
#ifndef _circle_spimasterdma_rp1_h
#define _circle_spimasterdma_rp1_h

#include <circle/interrupt.h>
#include <circle/dmachannel-rp1.h>
#include <circle/gpiopin.h>
#include <circle/types.h>

#ifndef _circle_spimasterdma_h
	#error Do not include this header file directly!
#endif

typedef void TSPICompletionRoutine (boolean bStatus, void *pParam);

class CSPIMasterDMA
{
public:
	const unsigned ChipSelectNone = 4;

public:
	CSPIMasterDMA (CInterruptSystem *pInterruptSystem,
		       unsigned nClockSpeed = 500000, unsigned CPOL = 0, unsigned CPHA = 0,
		       boolean bDMAChannelLite = TRUE, unsigned nDevice = 0);

	~CSPIMasterDMA (void);

	boolean Initialize (void);

	void SetClock (unsigned nClockSpeed);
	void SetMode (unsigned CPOL, unsigned CPHA);

	void SetCompletionRoutine (TSPICompletionRoutine *pRoutine, void *pParam);

	void StartWriteRead (unsigned nChipSelect, const void *pWriteBuffer, void *pReadBuffer,
			     unsigned nCount);

	int WriteReadSync (unsigned nChipSelect, const void *pWriteBuffer, void *pReadBuffer,
			   unsigned nCount);

private:
	void DMACompletionRoutine (unsigned nChannel, boolean bStatus);
	static void DMACompletionStub (unsigned nChannel, unsigned nBuffer,
				       boolean bStatus, void *pParam);

private:
	unsigned m_nDevice;
	uintptr  m_ulBaseAddress;
	boolean  m_bValid;

	CDMAChannelRP1 m_TxDMA;
	CDMAChannelRP1 m_RxDMA;

	CGPIOPin m_SCLK;
	CGPIOPin m_MOSI;
	CGPIOPin m_MISO;
	CGPIOPin m_CE0;
	CGPIOPin m_CE1;

	boolean m_bTxStatus;

	TSPICompletionRoutine *m_pCompletionRoutine;
	void *m_pCompletionParam;
};

#endif
