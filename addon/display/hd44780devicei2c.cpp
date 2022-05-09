//
// hd44780devicei2c.cpp
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
#include "hd44780devicei2c.h"
#include <circle/i2cmaster.h>
#include <circle/timer.h>
#include <assert.h>

enum TDisplayState
{
	ScreenStateStart,
	ScreenStateEscape,
	ScreenStateBracket,
	ScreenStateNumber1,
	ScreenStateQuestionMark,
	ScreenStateSemicolon,
	ScreenStateNumber2,
	ScreenStateNumber3,
	ScreenStateAutoPage
};

CHD44780Devicei2c::CHD44780Devicei2c (CI2CMaster *pI2CMaster, u8 nAddress, unsigned nColumns, unsigned nRows, boolean bBlockCursor)
:	m_nColumns (nColumns),
	m_nRows (nRows),
    m_pI2CMaster (pI2CMaster),
    m_nAddress (nAddress),
	m_bBlockCursor (bBlockCursor),
	m_nState (ScreenStateStart),
	m_SpinLock (TASK_LEVEL)
{
	assert (m_nColumns <= HD44780_MAX_COLUMNS);
	assert (m_nRows <= HD44780_MAX_ROWS);
}

CHD44780Devicei2c::~CHD44780Devicei2c (void)
{
	WriteByte (0x30);	// return to 8-bit mode
}

boolean CHD44780Devicei2c::Initialize (void)
{
	// set 4-bit mode, line count and font
	WriteHalfByte (0x02);
	WriteByte (m_nRows == 1 ? 0x20 : 0x28);

	SetCursorMode (TRUE);
	SetAutoPageMode (FALSE);
	CursorHome ();
	ClearDisplayEnd ();

	WriteByte (0x06);	// set move cursor right, do not shift display

	return TRUE;
}

unsigned CHD44780Devicei2c::GetColumns (void) const
{
	return m_nColumns;
}

unsigned CHD44780Devicei2c::GetRows (void) const
{
	return m_nRows;
}

int CHD44780Devicei2c::Write (const void *pBuffer, size_t nCount)
{
	const char *pChar = (const char *) pBuffer;
	assert (pChar != 0);

	int nResult = 0;

	m_SpinLock.Acquire ();

	while (nCount-- > 0)
	{
		Write (*pChar++);

		nResult++;
	}

	SetCursor ();

	m_SpinLock.Release ();

	return nResult;
}

void CHD44780Devicei2c::Write (char chChar)
{
	switch (m_nState)
	{
	case ScreenStateStart:
		switch (chChar)
		{
		case '\b':
			CursorLeft ();
			break;

		case '\t':
			Tabulator ();
			break;

		case '\n':
			NewLine ();
			break;

		case '\r':
			CarriageReturn ();
			break;

		case '\x1b':
			m_nState = ScreenStateEscape;
			break;

		default:
			DisplayChar (chChar);
			break;
		}
		break;

	case ScreenStateEscape:
		switch (chChar)
		{
		case '[':
			m_nState = ScreenStateBracket;
			break;

		case 'd':
			m_nState = ScreenStateAutoPage;
			break;

		default:
			m_nState = ScreenStateStart;
			break;
		}
		break;

	case ScreenStateBracket:
		switch (chChar)
		{
		case '?':
			m_nState = ScreenStateQuestionMark;
			break;

		case 'A':
			CursorUp ();
			m_nState = ScreenStateStart;
			break;

		case 'B':
			CursorDown ();
			m_nState = ScreenStateStart;
			break;

		case 'C':
			CursorRight ();
			m_nState = ScreenStateStart;
			break;

		case 'D':
			CursorLeft ();
			m_nState = ScreenStateStart;
			break;

		case 'H':
			CursorHome ();
			m_nState = ScreenStateStart;
			break;

		case 'J':
			ClearDisplayEnd ();
			m_nState = ScreenStateStart;
			break;

		case 'K':
			ClearLineEnd ();
			m_nState = ScreenStateStart;
			break;

		default:
			if ('0' <= chChar && chChar <= '9')
			{
				m_nParam1 = chChar - '0';
				m_nState = ScreenStateNumber1;
			}
			else
			{
				m_nState = ScreenStateStart;
			}
			break;
		}
		break;

	case ScreenStateNumber1:
		switch (chChar)
		{
		case ';':
			m_nState = ScreenStateSemicolon;
			break;

		case 'X':
			EraseChars (m_nParam1);
			m_nState = ScreenStateStart;
			break;

		default:
			if ('0' <= chChar && chChar <= '9')
			{
				m_nParam1 *= 10;
				m_nParam1 += chChar - '0';

				if (m_nParam1 > 99)
				{
					m_nState = ScreenStateStart;
				}
			}
			else
			{
				m_nState = ScreenStateStart;
			}
			break;
		}
		break;

	case ScreenStateSemicolon:
		if ('0' <= chChar && chChar <= '9')
		{
			m_nParam2 = chChar - '0';
			m_nState = ScreenStateNumber2;
		}
		else
		{
			m_nState = ScreenStateStart;
		}
		break;

	case ScreenStateQuestionMark:
		if ('0' <= chChar && chChar <= '9')
		{
			m_nParam1 = chChar - '0';
			m_nState = ScreenStateNumber3;
		}
		else
		{
			m_nState = ScreenStateStart;
		}
		break;

	case ScreenStateNumber2:
		switch (chChar)
		{
		case 'H':
			CursorMove (m_nParam1, m_nParam2);
			m_nState = ScreenStateStart;
			break;

		default:
			if ('0' <= chChar && chChar <= '9')
			{
				m_nParam2 *= 10;
				m_nParam2 += chChar - '0';

				if (m_nParam2 > 199)
				{
					m_nState = ScreenStateStart;
				}
			}
			else
			{
				m_nState = ScreenStateStart;
			}
			break;
		}
		break;

	case ScreenStateNumber3:
		switch (chChar)
		{
		case 'h':
		case 'l':
			if (m_nParam1 == 25)
			{
				SetCursorMode (chChar == 'h');
			}
			m_nState = ScreenStateStart;
			break;

		default:
			if ('0' <= chChar && chChar <= '9')
			{
				m_nParam1 *= 10;
				m_nParam1 += chChar - '0';

				if (m_nParam1 > 99)
				{
					m_nState = ScreenStateStart;
				}
			}
			else
			{
				m_nState = ScreenStateStart;
			}
			break;
		}
		break;

	case ScreenStateAutoPage:
		switch (chChar)
		{
		case '+':
			SetAutoPageMode (TRUE);
			m_nState = ScreenStateStart;
			break;

		case '*':
			SetAutoPageMode (FALSE);
			m_nState = ScreenStateStart;
			break;

		default:
			m_nState = ScreenStateStart;
			break;
		}
		break;

	default:
		m_nState = ScreenStateStart;
		break;
	}
}

void CHD44780Devicei2c::CarriageReturn (void)
{
	m_nCursorX = 0;
}

void CHD44780Devicei2c::ClearDisplayEnd (void)
{
	if (m_nCursorX == 0 && m_nCursorY == 0)
	{
		WriteByte (0x01);
		CTimer::SimpleMsDelay (2);

		for (unsigned nPosY = 0; nPosY < m_nRows; nPosY++)
		{
			for (unsigned nPosX = 0; nPosX < m_nColumns; nPosX++)
			{
				m_Buffer[nPosY][nPosX] = ' ';
			}
		}
	}
	else
	{
		ClearLineEnd ();

		for (unsigned nPosY = m_nCursorY+1; nPosY < m_nRows; nPosY++)
		{
			for (unsigned nPosX = 0; nPosX < m_nColumns; nPosX++)
			{
				SetChar (nPosX, nPosY, ' ');
			}
		}
	}
}

void CHD44780Devicei2c::ClearLineEnd (void)
{
	for (unsigned nPosX = m_nCursorX; nPosX < m_nColumns; nPosX++)
	{
		SetChar (nPosX, m_nCursorY, ' ');
	}
}

void CHD44780Devicei2c::CursorDown (void)
{
	if (++m_nCursorY >= m_nRows)
	{
		if (!m_bAutoPage)
		{
			Scroll ();

			m_nCursorY--;
		}
		else
		{
			m_nCursorY = 0;
		}
	}
}

void CHD44780Devicei2c::CursorHome (void)
{
	m_nCursorX = 0;
	m_nCursorY = 0;
}

void CHD44780Devicei2c::CursorLeft (void)
{
	if (m_nCursorX > 0)
	{
		m_nCursorX--;
	}
	else
	{
		if (m_nCursorY > 0)
		{
			m_nCursorX = m_nColumns-1;
			m_nCursorY--;
		}
	}
}

void CHD44780Devicei2c::CursorMove (unsigned nRow, unsigned nColumn)
{
	if (   --nColumn < m_nColumns
	    && --nRow    < m_nRows)
	{
		m_nCursorX = nColumn;
		m_nCursorY = nRow;
	}
}

void CHD44780Devicei2c::CursorRight (void)
{
	if (++m_nCursorX >= m_nColumns)
	{
		NewLine ();
	}
}

void CHD44780Devicei2c::CursorUp (void)
{
	if (m_nCursorY > 0)
	{
		m_nCursorY--;
	}
}

void CHD44780Devicei2c::DisplayChar (char chChar)
{
	u8 uchChar = (u8) chChar;

	if (' ' <= uchChar)
	{
		if (0x80 <= uchChar && uchChar <= 0x87)
		{
			uchChar -= 0x80;
		}

		SetChar (m_nCursorX, m_nCursorY, uchChar);

		CursorRight ();
	}
}

void CHD44780Devicei2c::EraseChars (unsigned nCount)
{
	if (nCount == 0)
	{
		return;
	}

	unsigned nEndX = m_nCursorX + nCount;
	if (nEndX > m_nColumns)
	{
		nEndX = m_nColumns;
	}

	for (unsigned nPosX = m_nCursorX; nPosX < nEndX; nPosX++)
	{
		SetChar (nPosX, m_nCursorY, ' ');
	}
}

void CHD44780Devicei2c::NewLine (void)
{
	CarriageReturn ();
	CursorDown ();
}

void CHD44780Devicei2c::SetAutoPageMode (boolean bEnable)
{
	m_bAutoPage = bEnable;
}

void CHD44780Devicei2c::SetCursorMode (boolean bVisible)
{
	m_bCursorOn = bVisible;

	WriteByte (m_bCursorOn ? (m_bBlockCursor ? 0x0D : 0x0E) : 0x0C);
}

void CHD44780Devicei2c::Tabulator (void)
{
	m_nCursorX = ((m_nCursorX + 8) / 8) * 8;
	if (m_nCursorX >= m_nColumns)
	{
		NewLine ();
	}
}

void CHD44780Devicei2c::Scroll (void)
{
	for (unsigned nPosY = 1; nPosY < m_nRows; nPosY++)
	{
		for (unsigned nPosX = 0; nPosX < m_nColumns; nPosX++)
		{
			SetChar (nPosX, nPosY-1, m_Buffer[nPosY][nPosX]);
		}
	}

	for (unsigned nPosX = 0; nPosX < m_nColumns; nPosX++)
	{
		SetChar (nPosX, m_nRows-1, ' ');
	}
}

void CHD44780Devicei2c::SetChar (unsigned nPosX, unsigned nPosY, char chChar)
{
	WriteByte (0x80 | GetDDRAMAddress (nPosX, nPosY));

	WriteCommandByte ((u8) chChar);

	m_Buffer[nPosY][nPosX] = chChar;
}

void CHD44780Devicei2c::SetCursor (void)
{
	if (m_bCursorOn)
	{
		WriteByte (0x80 | GetDDRAMAddress (m_nCursorX, m_nCursorY));
	}
}

void CHD44780Devicei2c::DefineCharFont (char chChar, const u8 FontData[8])
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
		WriteByte (0x40 | (uchCGRAMAddress + nLine));

		WriteCommandByte (FontData[nLine] & 0x1F);
	}
}

void CHD44780Devicei2c::WriteByte (u8 nData)
{
	WriteHalfByte (nData >> 4);
	WriteHalfByte (nData & 0x0F);
}

void CHD44780Devicei2c::WriteCommandByte (u8 nData)
{
	WriteCommandHalfByte (nData >> 4);
	WriteCommandHalfByte (nData & 0x0F);
}

#define LCD_DATA_BIT		(1<<0)
#define LCD_ENABLE_BIT		(1<<2)
#define LCD_BACKLIGHT_BIT	(1<<3)

void CHD44780Devicei2c::WriteHalfByte (u8 nData)
{
	u8 nByte = ((nData << 4) & 0xF0) | LCD_ENABLE_BIT;
	
	nByte |= LCD_BACKLIGHT_BIT;

	m_pI2CMaster->Write(m_nAddress, &nByte, 1);
	CTimer::SimpleusDelay (5);
	
	nByte &= ~LCD_ENABLE_BIT;
	m_pI2CMaster->Write(m_nAddress, &nByte, 1);

	CTimer::SimpleusDelay (100);
}

void CHD44780Devicei2c::WriteCommandHalfByte (u8 nData)
{
	u8 nByte = ((nData << 4) & 0xF0) | LCD_ENABLE_BIT;
	nByte |= LCD_BACKLIGHT_BIT;
	nByte |= LCD_DATA_BIT;

	m_pI2CMaster->Write(m_nAddress, &nByte, 1);
	CTimer::SimpleusDelay (5);
	
	nByte &= ~LCD_ENABLE_BIT;
	m_pI2CMaster->Write(m_nAddress, &nByte, 1);

	CTimer::SimpleusDelay (100);
}

u8 CHD44780Devicei2c::GetDDRAMAddress (unsigned nPosX, unsigned nPosY)
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
		uchAddress += m_nColumns;
		break;

	case 3:
		uchAddress += 0x40 + m_nColumns;
		break;

	default:
		assert (0);
		break;
	}

	return uchAddress;
}
