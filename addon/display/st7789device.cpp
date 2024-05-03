//
// st7789device.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2018-2022  R. Stange <rsta2@o2online.de>
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
#include <assert.h>

CST7789Device::CST7789Device (CSPIMaster *pSPIMaster, CST7789Display *pST7789Display,
		unsigned nColumns, unsigned nRows,
		boolean bBlockCursor)
:	CCharDevice (nColumns, nRows),
	m_pSPIMaster (pSPIMaster),
	m_pST7789Display (pST7789Display),
	m_bBlockCursor (bBlockCursor)
{
}

CST7789Device::~CST7789Device (void)
{
}

boolean CST7789Device::Initialize (void)
{
	return CCharDevice::Initialize();
}

void CST7789Device::DevClearCursor (void)
{
}

void CST7789Device::DevSetCursorMode (boolean bVisible)
{
}

void CST7789Device::DevSetChar (unsigned nPosX, unsigned nPosY, char chChar)
{
}

void CST7789Device::DevSetCursor (unsigned nCursorX, unsigned nCursorY)
{
}

void CST7789Device::DevUpdateDisplay (void)
{
}

void CST7789Device::DefineCharFont (char chChar, const u8 FontData[8])
{
}

void CST7789Device::WriteByte (u8 nData, int mode)
{
}

