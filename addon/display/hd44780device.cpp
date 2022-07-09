//
// hd44780device.cpp
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
#include "hd44780device.h"
#include <circle/timer.h>
#include <assert.h>

CHD44780Device::CHD44780Device (unsigned nColumns, unsigned nRows,
				unsigned nD4Pin, unsigned nD5Pin, unsigned nD6Pin, unsigned nD7Pin,
				unsigned nENPin, unsigned nRSPin, unsigned nRWPin,
				boolean bBlockCursor)
:	CCharDevice (nColumns, nRows),
	m_D4 (nD4Pin, GPIOModeOutput),
	m_D5 (nD5Pin, GPIOModeOutput),
	m_D6 (nD6Pin, GPIOModeOutput),
	m_D7 (nD7Pin, GPIOModeOutput),
	m_EN (nENPin, GPIOModeOutput),
	m_RS (nRSPin, GPIOModeOutput),
	m_pRW (0),
	m_pI2CMaster (0),
	m_nAddress (0),
	m_bBlockCursor (bBlockCursor)
{
	m_EN.Write (LOW);
	m_RS.Write (LOW);

	if (nRWPin != 0)
	{
		m_pRW = new CGPIOPin (nRWPin, GPIOModeOutput);
		if (m_pRW != 0)
		{
			m_pRW->Write (LOW);	// set to pin write, we do not read
		}
	}
}

CHD44780Device::CHD44780Device (CI2CMaster *pI2CMaster, u8 nAddress,
				unsigned nColumns, unsigned nRows, boolean bBlockCursor)
:	CCharDevice (nColumns, nRows),
	m_pI2CMaster (pI2CMaster),
	m_nAddress (nAddress),
	m_bBlockCursor (bBlockCursor)
{
	m_pRW = 0;
}

CHD44780Device::~CHD44780Device (void)
{
	WriteByte (0x30, HD44780_CMD);	// return to 8-bit mode

	if (m_pRW != 0) {
		delete m_pRW;
		m_pRW = 0;
	}
}

boolean CHD44780Device::Initialize (void)
{
	// set 4-bit mode, line count and font
	WriteHalfByte (0x02, HD44780_CMD);
	WriteByte (GetRows() == 1 ? 0x20 : 0x28, HD44780_CMD);

	WriteByte (0x06, HD44780_CMD);	// set move cursor right, do not shift display

	return CCharDevice::Initialize();
}

void CHD44780Device::DevClearCursor (void)
{
	WriteByte (0x01, HD44780_CMD);
	CTimer::SimpleMsDelay (2);
}

void CHD44780Device::DevSetCursorMode (boolean bVisible)
{
	WriteByte (bVisible ? (m_bBlockCursor ? 0x0D : 0x0E) : 0x0C, HD44780_CMD);
}

void CHD44780Device::DevSetChar (unsigned nPosX, unsigned nPosY, char chChar)
{
	WriteByte (0x80 | GetDDRAMAddress (nPosX, nPosY), HD44780_CMD);

	WriteByte ((u8) chChar, HD44780_DATA);
}

void CHD44780Device::DevSetCursor (unsigned nCursorX, unsigned nCursorY)
{
	WriteByte (0x80 | GetDDRAMAddress (nCursorX, nCursorY), HD44780_CMD);
}

void CHD44780Device::DevUpdateDisplay (void) {
}

void CHD44780Device::DefineCharFont (char chChar, const u8 FontData[8])
{
	u8 uchChar = (u8) chChar;
	if (   uchChar < 0x80
	    || uchChar > 0x87)
	{
		return;
	}
	uchChar -= 0x80;

	u8 uchCGRAMAddress = uchChar << 3;

	for (unsigned nLine = 0; nLine <= 7; nLine++)
	{
		WriteByte (0x40 | (uchCGRAMAddress + nLine), HD44780_CMD);

		WriteByte (FontData[nLine] & 0x1F, HD44780_DATA);
	}
}

void CHD44780Device::WriteByte (u8 nData, int mode)
{
	WriteHalfByte (nData >> 4, mode);
	WriteHalfByte (nData & 0x0F, mode);
}

#define LCD_DATA_BIT		(1<<0)
#define LCD_ENABLE_BIT		(1<<2)
#define LCD_BACKLIGHT_BIT	(1<<3)

void CHD44780Device::WriteHalfByte (u8 nData, int mode)
{
	if (m_pI2CMaster) {
		// I2C interface
		u8 nByte = ((nData << 4) & 0xF0) | LCD_ENABLE_BIT;
		nByte |= LCD_BACKLIGHT_BIT;
		if (mode == HD44780_DATA) {
			nByte |= LCD_DATA_BIT;
		}

		m_pI2CMaster->Write(m_nAddress, &nByte, 1);
		CTimer::SimpleusDelay (5);

		nByte &= ~LCD_ENABLE_BIT;
		m_pI2CMaster->Write(m_nAddress, &nByte, 1);

		CTimer::SimpleusDelay (100);
	} else {
		// 4-pin data interface
		
		if (mode == HD44780_DATA) {
			m_RS.Write(HIGH);
		}
		m_D4.Write (nData & 1 ? HIGH : LOW);
		m_D5.Write (nData & 2 ? HIGH : LOW);
		m_D6.Write (nData & 4 ? HIGH : LOW);
		m_D7.Write (nData & 8 ? HIGH : LOW);

		m_EN.Write (HIGH);
		CTimer::SimpleusDelay (1);
		m_EN.Write (LOW);
		CTimer::SimpleusDelay (50);
		if (mode == HD44780_DATA) {
			m_RS.Write(LOW);
		}
	}
}

u8 CHD44780Device::GetDDRAMAddress (unsigned nPosX, unsigned nPosY)
{
	u8 uchAddress = nPosX;
	switch (nPosY)
	{
	case 0:
		break;

	case 1:
		uchAddress += 0x40;
		break;

	case 2:
		uchAddress += GetColumns ();
		break;

	case 3:
		uchAddress += 0x40 + GetColumns ();
		break;

	default:
		assert (0);
		break;
	}

	return uchAddress;
}
