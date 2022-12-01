//
// chardevice.cpp
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
#include "chardevice.h"
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

CCharDevice::CCharDevice (unsigned nColumns, unsigned nRows)
:	m_nColumns (nColumns),
	m_nRows (nRows),
	m_nState (ScreenStateStart),
	m_SpinLock (TASK_LEVEL)
{
	assert (m_nColumns <= CHAR_MAX_COLUMNS);
	assert (m_nRows <= CHAR_MAX_ROWS);
}

CCharDevice::~CCharDevice (void)
{
}

boolean CCharDevice::Initialize (void)
{
	SetCursorMode (TRUE);
	SetAutoPageMode (FALSE);
	CursorHome ();
	ClearDisplayEnd ();

	return TRUE;
}

unsigned CCharDevice::GetColumns (void) const
{
	return m_nColumns;
}

unsigned CCharDevice::GetRows (void) const
{
	return m_nRows;
}

int CCharDevice::Write (const void *pBuffer, size_t nCount)
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

	DevUpdateDisplay ();

	m_SpinLock.Release ();

	return nResult;
}

void CCharDevice::Write (char chChar)
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

void CCharDevice::CarriageReturn (void)
{
	m_nCursorX = 0;
}

void CCharDevice::ClearDisplayEnd (void)
{
	if (m_nCursorX == 0 && m_nCursorY == 0)
	{
		DevClearCursor ();
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

void CCharDevice::ClearLineEnd (void)
{
	for (unsigned nPosX = m_nCursorX; nPosX < m_nColumns; nPosX++)
	{
		SetChar (nPosX, m_nCursorY, ' ');
	}
}

void CCharDevice::CursorDown (void)
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

void CCharDevice::CursorHome (void)
{
	m_nCursorX = 0;
	m_nCursorY = 0;
}

void CCharDevice::CursorLeft (void)
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

void CCharDevice::CursorMove (unsigned nRow, unsigned nColumn)
{
	if (   --nColumn < m_nColumns
	    && --nRow    < m_nRows)
	{
		m_nCursorX = nColumn;
		m_nCursorY = nRow;
	}
}

void CCharDevice::CursorRight (void)
{
	if (++m_nCursorX >= m_nColumns)
	{
		NewLine ();
	}
}

void CCharDevice::CursorUp (void)
{
	if (m_nCursorY > 0)
	{
		m_nCursorY--;
	}
}

void CCharDevice::DisplayChar (char chChar)
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

void CCharDevice::EraseChars (unsigned nCount)
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

void CCharDevice::NewLine (void)
{
	CarriageReturn ();
	CursorDown ();
}

void CCharDevice::SetAutoPageMode (boolean bEnable)
{
	m_bAutoPage = bEnable;
}

void CCharDevice::SetCursorMode (boolean bVisible)
{
	m_bCursorOn = bVisible;
	
	DevSetCursorMode (bVisible);

}

void CCharDevice::Tabulator (void)
{
	m_nCursorX = ((m_nCursorX + 8) / 8) * 8;
	if (m_nCursorX >= m_nColumns)
	{
		NewLine ();
	}
}

void CCharDevice::Scroll (void)
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

void CCharDevice::SetChar (unsigned nPosX, unsigned nPosY, char chChar)
{
	DevSetChar(nPosX, nPosY, chChar);
	m_Buffer[nPosY][nPosX] = chChar;
}

void CCharDevice::SetCursor (void)
{
	if (m_bCursorOn)
	{
		DevSetCursor (m_nCursorX, m_nCursorY);
	}
}

