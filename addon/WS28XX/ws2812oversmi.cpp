//
// \file ws2812oversmi.cpp
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
#include "ws2812oversmi.h"
#include <circle/util.h>


CWS2812OverSMI::CWS2812OverSMI(CInterruptSystem *pInterruptSystem, unsigned nSDLinesMask, unsigned nNumberOfLEDsPerStrip) :
		m_SMIMaster (pInterruptSystem, nSDLinesMask, FALSE),
		m_nLEDCount (nNumberOfLEDsPerStrip),
		m_bDirty (TRUE)
{
	assert (m_nLEDCount > 0);
	unsigned len = TX_BUFF_LEN(m_nLEDCount);
	m_pBuffer = new TXDATA_T[len]; // using new makes the buffer cache-aligned, so suitable for DMA
	memset(m_pBuffer, 0, len * sizeof(TXDATA_T));
	for (unsigned nStripIndex = 0; nStripIndex < LED_NCHANS; nStripIndex++) {
		if (nSDLinesMask & (1 << nStripIndex)) {
			for (unsigned nLEDIndex = 0; nLEDIndex < m_nLEDCount; nLEDIndex++) SetLED (nStripIndex, nLEDIndex, 0, 0, 0);
		}
	}
	m_SMIMaster.SetupTiming(NEOPIXEL_SMI_WIDTH, NEOPIXEL_SMI_NS, NEOPIXEL_SMI_SETUP, NEOPIXEL_SMI_STROBE, NEOPIXEL_SMI_HOLD, NEOPIXEL_SMI_PACE);
	m_SMIMaster.SetupDMA(m_pBuffer, len * sizeof(TXDATA_T));
}


CWS2812OverSMI::~CWS2812OverSMI() {
	delete[] m_pBuffer;
}


unsigned CWS2812OverSMI::GetLEDCount() const {
	return m_nLEDCount;
}


void CWS2812OverSMI::Update() {
	if (!m_bDirty) return;
	m_bDirty = FALSE;
	m_SMIMaster.WriteDMA(FALSE);
}

void CWS2812OverSMI::SetLED(unsigned nSDLine, unsigned nLEDIndexInStrip, u8 nRed, u8 nGreen, u8 nBlue) {
	assert(nSDLine < LED_NCHANS);
	assert(nLEDIndexInStrip < m_nLEDCount);
	m_bDirty = TRUE;
	TXDATA_T *txd = &m_pBuffer[LED_TX_OSET(nLEDIndexInStrip)];
	unsigned grb = (((unsigned)nGreen)<<16) + (((unsigned)nRed)<<8) + (((unsigned)nBlue)<<0);

	// Logic 1 is 0.8us high, 0.4 us low, logic 0 is 0.4us high, 0.8us low
	TXDATA_T nLineMask = 1 << nSDLine;
	boolean onlyOneLine = __builtin_popcount(m_SMIMaster.GetSDLinesMask()) == 1;
	// For each bit of the 24-bit GRB values...
	for (unsigned msk = 1<<(LED_NBITS-1); msk > 0; msk >>= 1) {
		// 1st byte or word is a high pulse on all lines
		txd[0] = (TXDATA_T) 0xffff;
		// 2nd has high or low bits from data
		if (onlyOneLine) {
			if (grb & msk) txd[1] = nLineMask;
			else txd[1] = 0;
		}
		else {
			// todo this OR/AND mechanism below does not work - because of the data cache?
			// If so we'd need to maintain a non-cached copy of m_pBuffer and flush it into the DMA buffer in Update()
			if (grb & msk) txd[1] |= nLineMask;
			else txd[1] &= ~nLineMask;
		}
		// 3rd is a low pulse on all lines
		txd[2] = 0;
		txd += BIT_NPULSES;
	}
}
