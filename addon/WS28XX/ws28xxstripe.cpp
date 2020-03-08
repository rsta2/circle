//
// ws28xxstripe.h
//
// Driver for WS28XX controlled LED stripes
// Original development by Arjan van Vught <info@raspberrypi-dmx.nl>
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2020  R. Stange <rsta2@o2online.de>
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
#include "ws28xxstripe.h"
#include <circle/util.h>
#include <assert.h>

CWS28XXStripe::CWS28XXStripe (TWS28XXType Type, unsigned nLEDCount, unsigned nClockSpeed,
			      unsigned nSPIDevice)
:	m_Type (Type),
	m_nLEDCount (nLEDCount),
	m_SPIMaster (m_Type == WS2801 ? nClockSpeed : 6400000, 0, 0, nSPIDevice)
{
	assert (m_Type <= WS2812B);
	assert (m_nLEDCount > 0);

	m_nBufSize = m_nLEDCount * 3;
	if (   m_Type == WS2812
	    || m_Type == WS2812B)
	{
		m_nBufSize *= 8;
	}
	
	m_pBuffer = new u8[m_nBufSize];
	assert (m_pBuffer != 0);

	for (unsigned nLEDIndex = 0; nLEDIndex < m_nLEDCount; nLEDIndex++)
	{
		SetLED (nLEDIndex, 0, 0, 0);
	}

	m_pBlackoutBuffer = new u8[m_nBufSize];
	assert (m_pBlackoutBuffer != 0);
	memset (m_pBlackoutBuffer, m_Type == WS2801 ? 0 : 0xC0, m_nBufSize);
}

CWS28XXStripe::~CWS28XXStripe (void)
{
	delete [] m_pBlackoutBuffer;
	m_pBlackoutBuffer = 0;

	delete [] m_pBuffer;
	m_pBuffer = 0;
}

boolean CWS28XXStripe::Initialize (void)
{
	return m_SPIMaster.Initialize ();
}

unsigned CWS28XXStripe::GetLEDCount (void) const
{
	return m_nLEDCount;
}

void CWS28XXStripe::SetLED (unsigned nLEDIndex, u8 nRed, u8 nGreen, u8 nBlue)
{
	assert (m_pBuffer != 0);
	assert (nLEDIndex < m_nLEDCount);
	unsigned nOffset = nLEDIndex * 3;

	if (m_Type == WS2801)
	{
		assert (nOffset+2 < m_nBufSize);
		m_pBuffer[nOffset]   = nRed;
		m_pBuffer[nOffset+1] = nGreen;
		m_pBuffer[nOffset+2] = nBlue;
	}
	else
	{
		nOffset *= 8;

		SetColorWS2812 (nOffset,    nGreen);
		SetColorWS2812 (nOffset+8,  nRed);
		SetColorWS2812 (nOffset+16, nBlue);
	}
}

boolean CWS28XXStripe::Update (void)
{
	assert (m_pBuffer != 0);
	return m_SPIMaster.Write (0, m_pBuffer, m_nBufSize) == (int) m_nBufSize;
}

boolean CWS28XXStripe::Blackout (void)
{
	assert (m_pBlackoutBuffer != 0);
	return m_SPIMaster.Write (0, m_pBlackoutBuffer, m_nBufSize) == (int) m_nBufSize;
}

void CWS28XXStripe::SetColorWS2812 (unsigned nOffset, u8 nValue)
{
	assert (m_Type != WS2801);
	u8 nHighCode = m_Type == WS2812 ? 0xF0 : 0xF8;

	assert (nOffset+7 < m_nBufSize);

	for (u8 nMask = 0x80; nMask != 0; nMask >>= 1)
	{
		if (nValue & nMask)
		{
			m_pBuffer[nOffset] = nHighCode;
		}
		else
		{
			m_pBuffer[nOffset] = 0xC0;
		}

		nOffset++;
	}
}
