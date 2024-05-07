//
// st7789device.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2018-2024  R. Stange <rsta2@o2online.de>
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
#include "st7789device.h"
#include <circle/timer.h>
#include <circle/chargenerator.h>
#include <assert.h>

CST7789Device::CST7789Device (CSPIMaster *pSPIMaster, CST7789Display *pST7789Display,
		unsigned nColumns, unsigned nRows, bool bDoubleWidth, bool bDoubleHeight,
		boolean bBlockCursor)
:	CCharDevice (nColumns, nRows),
	m_pSPIMaster (pSPIMaster),
	m_pST7789Display (pST7789Display),
	m_nColumns (nColumns),
	m_nRows (nRows),
	m_bDoubleWidth (bDoubleWidth),
	m_bDoubleHeight (bDoubleHeight),
	m_bBlockCursor (bBlockCursor)
{
}

CST7789Device::~CST7789Device (void)
{
}

boolean CST7789Device::Initialize (void)
{
	// ST7789 display assumed to already be initialised prior to
	// initialising the character device, so nothing more to be
	// done here other than checking the dimensions are sensible...
	
	unsigned r = m_pST7789Display->GetRotation();
	unsigned w = m_pST7789Display->GetWidth();
	unsigned h = m_pST7789Display->GetHeight();	
	if (r==90 || r==270) {
		// Swap w/h if rotated
		w = m_pST7789Display->GetHeight();
		h = m_pST7789Display->GetWidth();
	}
	
	// st7789display uses the chargenerator, so check some properties here.
	// NB: By default it uses width/height x 2, but that can be changed
	//     for the width if required.
	CCharGenerator cg;
	m_nCharW = cg.GetCharWidth() * (m_bDoubleWidth ? 2 : 1);
	m_nCharH = cg.GetCharHeight() * (m_bDoubleHeight ? 2 : 1);

	if (m_nColumns * m_nCharW > w)
	{
		// Limit number of columns to width of screen
		m_nColumns = w / m_nCharW;
	}
	if (m_nRows * m_nCharH > h)
	{
		// Limit number of rows to height of screen
		m_nRows = h / m_nCharH;
	}
	
	m_pST7789Display->Clear(ST7789_BLACK_COLOR);
	m_pST7789Display->On();

	return CCharDevice::Initialize();
}

void CST7789Device::DevClearCursor (void)
{
	// Just clear the display
	m_pST7789Display->Clear(ST7789_BLACK_COLOR);
}

void CST7789Device::DevSetCursorMode (boolean bVisible)
{
}

void CST7789Device::DevSetChar (unsigned nPosX, unsigned nPosY, char chChar)
{
	char s[2];
	s[0] = chChar;
	s[1] = '\0';

	if ((nPosX >= m_nColumns) || (nPosY >= m_nRows))
	{
		// Off the display so quit
		return;
	}

	// Convert from cursor coordinates to pixel coordinates
	unsigned nXC = nPosX * m_nCharW;
	unsigned nYC = nPosY * m_nCharH;

	m_pST7789Display->DrawText(nXC, nYC, s, ST7789_WHITE_COLOR, ST7789_BLACK_COLOR, m_bDoubleWidth, m_bDoubleHeight);
}

void CST7789Device::DevSetCursor (unsigned nCursorX, unsigned nCursorY)
{
}

void CST7789Device::DevUpdateDisplay (void)
{
}
