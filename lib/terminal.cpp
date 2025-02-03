//
// terminal.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2025  R. Stange <rsta2@o2online.de>
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
#include <circle/terminal.h>
#include <circle/devicenameservice.h>
#include <circle/synchronize.h>
#include <circle/sysconfig.h>
#include <circle/timer.h>
#include <circle/util.h>

static const char DevicePrefix[] = "tty";

CTerminalDevice::CTerminalDevice (CDisplay *pDisplay, unsigned nDeviceIndex,
				  const TFont &rFont, CCharGenerator::TFontFlags FontFlags)
:	m_pDisplay (pDisplay),
	m_nDeviceIndex (nDeviceIndex),
	m_CharGen (rFont, FontFlags),
	m_pCursorPixels (nullptr),
	m_pBuffer8 (nullptr),
	m_nSize (0),
	m_nPitch (0),
	m_nWidth (0),
	m_nHeight (0),
	m_nUsedWidth (0),
	m_nUsedHeight (0),
	m_nDepth (0),
	m_State (StateStart),
	m_nScrollStart (0),
	m_nScrollEnd (0),
	m_nCursorX (0),
	m_nCursorY (0),
	m_bCursorOn (TRUE),
	m_bCursorBlock (FALSE),
	m_bCursorVisible (FALSE),
	m_Color (0),
	m_BackgroundColor (0),
	m_bReverseAttribute (FALSE),
	m_bInsertOn (FALSE),
	m_bAutoPage (FALSE),
	m_bDelayedUpdate (FALSE)
#ifdef REALTIME
	, m_SpinLock (TASK_LEVEL)
#endif
{
}

CTerminalDevice::~CTerminalDevice (void)
{
	CDeviceNameService::Get ()->RemoveDevice (DevicePrefix, m_nDeviceIndex+1, FALSE);

	delete [] m_pBuffer8;
	m_pBuffer8 = nullptr;

	delete [] m_pCursorPixels;
	m_pCursorPixels = nullptr;

	m_pDisplay = nullptr;
}

boolean CTerminalDevice::Initialize (void)
{
	m_nWidth  = m_pDisplay->GetWidth ();
	m_nHeight = m_pDisplay->GetHeight ();
	m_nDepth  = m_pDisplay->GetDepth ();
	m_nSize   = m_nWidth * m_nHeight * m_nDepth/8;
	m_nPitch  = m_nWidth * m_nDepth/8;

	if (   m_nDepth == 1
	    && m_nWidth % 8 != 0)
	{
		return FALSE;
	}

	m_pBuffer8 = new u8[m_nSize];
	if (!m_pBuffer8)
	{
		return FALSE;
	}

	m_pCursorPixels = new CDisplay::TRawColor[  m_CharGen.GetCharWidth ()
						  * m_CharGen.GetCharHeight ()];
	if (!m_pCursorPixels)
	{
		return FALSE;
	}

	m_nUsedWidth = m_nWidth / m_CharGen.GetCharWidth () * m_CharGen.GetCharWidth ();
	m_nUsedHeight = m_nHeight / m_CharGen.GetCharHeight () * m_CharGen.GetCharHeight ();
	m_nScrollEnd = m_nUsedHeight;

	m_Color = m_pDisplay->GetColor (CDisplay::NormalColor);
	m_BackgroundColor = m_pDisplay->GetColor (CDisplay::Black);

	CursorHome ();
	ClearDisplayEnd ();
	InvertCursor ();

	// Initial update
	m_UpdateArea.x1 = 0;
	m_UpdateArea.x2 = m_nWidth-1;
	m_UpdateArea.y1 = 0;
	m_UpdateArea.y2 = m_nHeight-1;
	m_pDisplay->SetArea (m_UpdateArea, m_pBuffer8);

	m_UpdateArea.y1 = m_nHeight;
	m_UpdateArea.y2 = 0;

	if (!CDeviceNameService::Get ()->GetDevice (DevicePrefix, m_nDeviceIndex+1, FALSE))
	{
		CDeviceNameService::Get ()->AddDevice (DevicePrefix, m_nDeviceIndex+1, this, FALSE);
	}

	return TRUE;
}

unsigned CTerminalDevice::GetWidth (void) const
{
	return m_nWidth;
}

unsigned CTerminalDevice::GetHeight (void) const
{
	return m_nHeight;
}

unsigned CTerminalDevice::GetColumns (void) const
{
	return m_nUsedWidth / m_CharGen.GetCharWidth ();
}

unsigned CTerminalDevice::GetRows (void) const
{
	return m_nUsedHeight / m_CharGen.GetCharHeight ();
}

CDisplay *CTerminalDevice::GetDisplay (void)
{
	return m_pDisplay;
}

int CTerminalDevice::Write (const void *pBuffer, size_t nCount)
{
#ifdef REALTIME
	// cannot write from IRQ_LEVEL to prevent deadlock, just ignore it
	if (CurrentExecutionLevel () > TASK_LEVEL)
	{
		return nCount;
	}
#endif

	m_SpinLock.Acquire ();

	InvertCursor ();

	const char *pChar = (const char *) pBuffer;
	int nResult = 0;

	while (nCount--)
	{
		Write (*pChar++);

		nResult++;
	}

	InvertCursor ();

	// Update display
	if (   !m_bDelayedUpdate
	    && m_UpdateArea.y1 <= m_UpdateArea.y2)
	{
		m_pDisplay->SetArea (m_UpdateArea, m_pBuffer8 + m_UpdateArea.y1 * m_nPitch);

		m_UpdateArea.y1 = m_nHeight;
		m_UpdateArea.y2 = 0;
	}

	m_SpinLock.Release ();

	return nResult;
}

void CTerminalDevice::SetPixel (unsigned nPosX, unsigned nPosY, TTerminalColor Color)
{
	if (   nPosX >= m_nWidth
	    || nPosY >= m_nHeight)
	{
		return;
	}

	CDisplay::TRawColor nColor = m_pDisplay->GetColor (Color);

	SetRawPixel (nPosX, nPosY, nColor);

	m_pDisplay->SetPixel (nPosX, nPosY, nColor);
}

void CTerminalDevice::SetPixel (unsigned nPosX, unsigned nPosY, CDisplay::TRawColor nColor)
{
	if (   nPosX >= m_nWidth
	    || nPosY >= m_nHeight)
	{
		return;
	}

	SetRawPixel (nPosX, nPosY, nColor);

	m_pDisplay->SetPixel (nPosX, nPosY, nColor);
}

TTerminalColor CTerminalDevice::GetPixel (unsigned nPosX, unsigned nPosY)
{
	if (   nPosX >= m_nWidth
	    || nPosY >= m_nHeight)
	{
		return CDisplay::Black;
	}

	return m_pDisplay->GetColor (GetRawPixel (nPosX, nPosY));
}

void CTerminalDevice::SetCursorBlock (boolean bCursorBlock)
{
	m_bCursorBlock = bCursorBlock;
}

void CTerminalDevice::Update (unsigned nMillis)
{
	if (!m_bDelayedUpdate)
	{
		m_bDelayedUpdate = TRUE;

		m_nLastUpdateTicks = CTimer::GetClockTicks ();

		return;
	}

	m_SpinLock.Acquire ();

	unsigned nTicks = CTimer::GetClockTicks ();

	if (   m_UpdateArea.y1 <= m_UpdateArea.y2
	    && (   !nMillis
		|| nTicks - m_nLastUpdateTicks >= nMillis * CLOCKHZ / 1000))
	{
		m_pDisplay->SetArea (m_UpdateArea, m_pBuffer8 + m_UpdateArea.y1 * m_nPitch);

		m_UpdateArea.y1 = m_nHeight;
		m_UpdateArea.y2 = 0;

		m_nLastUpdateTicks = nTicks;
	}

	m_SpinLock.Release ();
}

void CTerminalDevice::Write (char chChar)
{
	switch (m_State)
	{
	case StateStart:
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
			m_State = StateEscape;
			break;

		default:
			DisplayChar (chChar);
			break;
		}
		break;

	case StateEscape:
		switch (chChar)
		{
		case 'M':
			ReverseScroll ();
			m_State = StateStart;
			break;

		case '[':
			m_State = StateBracket;
			break;

		case 'd':
			m_State = StateAutoPage;
			break;

		default:
			m_State = StateStart;
			break;
		}
		break;

	case StateBracket:
		switch (chChar)
		{
		case '?':
			m_State = StateQuestionMark;
			break;

		case 'A':
			CursorUp ();
			m_State = StateStart;
			break;

		case 'B':
			CursorDown ();
			m_State = StateStart;
			break;

		case 'C':
			CursorRight ();
			m_State = StateStart;
			break;

		case 'D':
			CursorLeft ();
			m_State = StateStart;
			break;

		case 'H':
			CursorHome ();
			m_State = StateStart;
			break;

		case 'J':
			ClearDisplayEnd ();
			m_State = StateStart;
			break;

		case 'K':
			ClearLineEnd ();
			m_State = StateStart;
			break;

		case 'L':
			InsertLines (1);
			m_State = StateStart;
			break;

		case 'M':
			DeleteLines (1);
			m_State = StateStart;
			break;

		case 'P':
			DeleteChars (1);
			m_State = StateStart;
			break;

		default:
			if ('0' <= chChar && chChar <= '9')
			{
				m_nParam1 = chChar - '0';
				m_State = StateNumber1;
			}
			else
			{
				m_State = StateStart;
			}
			break;
		}
		break;

	case StateNumber1:
		switch (chChar)
		{
		case ';':
			m_State = StateSemicolon;
			break;

		case 'L':
			InsertLines (m_nParam1);
			m_State = StateStart;
			break;

		case 'M':
			DeleteLines (m_nParam1);
			m_State = StateStart;
			break;

		case 'P':
			DeleteChars (m_nParam1);
			m_State = StateStart;
			break;

		case 'X':
			EraseChars (m_nParam1);
			m_State = StateStart;
			break;

		case 'h':
		case 'l':
			if (m_nParam1 == 4)
			{
				InsertMode (chChar == 'h');
			}
			m_State = StateStart;
			break;

		case 'm':
			SetStandoutMode (m_nParam1);
			m_State = StateStart;
			break;

		default:
			if ('0' <= chChar && chChar <= '9')
			{
				m_nParam1 *= 10;
				m_nParam1 += chChar - '0';

				if (m_nParam1 > 199)
				{
					m_State = StateStart;
				}
			}
			else
			{
				m_State = StateStart;
			}
			break;
		}
		break;

	case StateSemicolon:
		if ('0' <= chChar && chChar <= '9')
		{
			m_nParam2 = chChar - '0';
			m_State = StateNumber2;
		}
		else
		{
			m_State = StateStart;
		}
		break;

	case StateQuestionMark:
		if ('0' <= chChar && chChar <= '9')
		{
			m_nParam1 = chChar - '0';
			m_State = StateNumber3;
		}
		else
		{
			m_State = StateStart;
		}
		break;

	case StateNumber2:
		switch (chChar)
		{
		case 'H':
			CursorMove (m_nParam1, m_nParam2);
			m_State = StateStart;
			break;

		case 'r':
			SetScrollRegion (m_nParam1, m_nParam2);
			m_State = StateStart;
			break;

		default:
			if ('0' <= chChar && chChar <= '9')
			{
				m_nParam2 *= 10;
				m_nParam2 += chChar - '0';

				if (m_nParam2 > 199)
				{
					m_State = StateStart;
				}
			}
			else
			{
				m_State = StateStart;
			}
			break;
		}
		break;

	case StateNumber3:
		switch (chChar)
		{
		case 'h':
		case 'l':
			if (m_nParam1 == 25)
			{
				SetCursorMode (chChar == 'h');
			}
			m_State = StateStart;
			break;

		default:
			if ('0' <= chChar && chChar <= '9')
			{
				m_nParam1 *= 10;
				m_nParam1 += chChar - '0';

				if (m_nParam1 > 99)
				{
					m_State = StateStart;
				}
			}
			else
			{
				m_State = StateStart;
			}
			break;
		}
		break;

	case StateAutoPage:
		switch (chChar)
		{
		case '+':
			SetAutoPageMode (TRUE);
			m_State = StateStart;
			break;

		case '*':
			SetAutoPageMode (FALSE);
			m_State = StateStart;
			break;

		default:
			m_State = StateStart;
			break;
		}
		break;

	default:
		m_State = StateStart;
		break;
	}
}

void CTerminalDevice::CarriageReturn (void)
{
	m_nCursorX = 0;
}

void CTerminalDevice::ClearDisplayEnd (void)
{
	ClearLineEnd ();

	unsigned nPosY = m_nCursorY + m_CharGen.GetCharHeight ();
	unsigned nOffset = nPosY * m_nWidth;

	switch (m_nDepth)
	{
	case 1: {
			nOffset /= 8;
			unsigned nSize = m_nSize - nOffset;
			for (u8 *pBuffer = m_pBuffer8 + nOffset; nSize--;)
			{
				*pBuffer++ = m_BackgroundColor ? 0xFF : 0;
			}
		} break;

	case 8: {
			unsigned nSize = m_nSize - nOffset;
			for (u8 *pBuffer = m_pBuffer8 + nOffset; nSize--;)
			{
				*pBuffer++ = (u8) m_BackgroundColor;
			}
		} break;

	case 16: {
			unsigned nSize = m_nSize / 2 - nOffset;
			for (u16 *pBuffer = m_pBuffer16 + nOffset; nSize--;)
			{
				*pBuffer++ = (u16) m_BackgroundColor;
			}
		} break;

	case 32: {
			unsigned nSize = m_nSize / 4 - nOffset;
			for (u32 *pBuffer = m_pBuffer32 + nOffset; nSize--;)
			{
				*pBuffer++ = (u32) m_BackgroundColor;
			}
		} break;
	}

	SetUpdateArea (m_nCursorY, m_nHeight-1);
}

void CTerminalDevice::ClearLineEnd (void)
{
	for (unsigned nPosX = m_nCursorX; nPosX < m_nUsedWidth; nPosX += m_CharGen.GetCharWidth ())
	{
		EraseChar (nPosX, m_nCursorY);
	}

	for (unsigned nPosX = m_nUsedWidth; nPosX < m_nWidth; nPosX++)
	{
		for (unsigned nPosY = m_nCursorY;
		     nPosY < m_nCursorY + m_CharGen.GetCharHeight (); nPosY++)
		{
			SetRawPixel (nPosX, nPosY, m_BackgroundColor);
		}
	}
}

void CTerminalDevice::CursorDown (void)
{
	m_nCursorY += m_CharGen.GetCharHeight ();
	if (m_nCursorY >= m_nScrollEnd)
	{
		if (!m_bAutoPage)
		{
			Scroll ();

			m_nCursorY -= m_CharGen.GetCharHeight ();
		}
		else
		{
			m_nCursorY = 0;
		}
	}
}

void CTerminalDevice::CursorHome (void)
{
	m_nCursorX = 0;
	m_nCursorY = m_nScrollStart;
}

void CTerminalDevice::CursorLeft (void)
{
	if (m_nCursorX > 0)
	{
		m_nCursorX -= m_CharGen.GetCharWidth ();
	}
	else
	{
		if (m_nCursorY > m_nScrollStart)
		{
			m_nCursorX = m_nUsedWidth - m_CharGen.GetCharWidth ();
			m_nCursorY -= m_CharGen.GetCharHeight ();
		}
	}
}

void CTerminalDevice::CursorMove (unsigned nRow, unsigned nColumn)
{
	unsigned nPosX = (nColumn - 1) * m_CharGen.GetCharWidth ();
	unsigned nPosY = (nRow - 1) * m_CharGen.GetCharHeight ();

	if (   nPosX < m_nUsedWidth
	    && nPosY >= m_nScrollStart
	    && nPosY < m_nScrollEnd)
	{
		m_nCursorX = nPosX;
		m_nCursorY = nPosY;
	}
}

void CTerminalDevice::CursorRight (void)
{
	m_nCursorX += m_CharGen.GetCharWidth ();
	if (m_nCursorX >= m_nUsedWidth)
	{
		NewLine ();
	}
}

void CTerminalDevice::CursorUp (void)
{
	if (m_nCursorY > m_nScrollStart)
	{
		m_nCursorY -= m_CharGen.GetCharHeight ();
	}
}

void CTerminalDevice::DeleteChars (unsigned nCount)	// TODO
{
}

void CTerminalDevice::DeleteLines (unsigned nCount)	// TODO
{
}

void CTerminalDevice::DisplayChar (char chChar)
{
	// TODO: Insert mode

	if (' ' <= (unsigned char) chChar)
	{
		DisplayChar (chChar, m_nCursorX, m_nCursorY, GetTextColor ());

		CursorRight ();
	}
}

void CTerminalDevice::EraseChars (unsigned nCount)
{
	if (nCount == 0)
	{
		return;
	}

	unsigned nEndX = m_nCursorX + nCount * m_CharGen.GetCharWidth ();
	if (nEndX > m_nUsedWidth)
	{
		nEndX = m_nUsedWidth;
	}

	for (unsigned nPosX = m_nCursorX; nPosX < nEndX; nPosX += m_CharGen.GetCharWidth ())
	{
		EraseChar (nPosX, m_nCursorY);
	}
}

CDisplay::TRawColor CTerminalDevice::GetTextBackgroundColor (void)
{
	return m_bReverseAttribute ? m_Color : m_BackgroundColor;
}

CDisplay::TRawColor CTerminalDevice::GetTextColor (void)
{
	return m_bReverseAttribute ? m_BackgroundColor : m_Color;
}

void CTerminalDevice::InsertLines (unsigned nCount)	// TODO
{
}

void CTerminalDevice::InsertMode (boolean bBegin)
{
	m_bInsertOn = bBegin;
}

void CTerminalDevice::NewLine (void)
{
	CarriageReturn ();
	CursorDown ();
}

void CTerminalDevice::ReverseScroll (void)
{
	if (   m_nCursorX == 0
	    && m_nCursorY == 0
	)
	{
		InsertLines (1);
	}
}

void CTerminalDevice::SetAutoPageMode (boolean bEnable)
{
	m_bAutoPage = bEnable;
}

void CTerminalDevice::SetCursorMode (boolean bVisible)
{
	m_bCursorOn = bVisible;
}

void CTerminalDevice::SetScrollRegion (unsigned nStartRow, unsigned nEndRow)
{
	unsigned nScrollStart = (nStartRow - 1) * m_CharGen.GetCharHeight ();
	unsigned nScrollEnd   = nEndRow * m_CharGen.GetCharHeight ();

	if (   nScrollStart <  m_nUsedHeight
	    && nScrollEnd   >  0
	    && nScrollEnd   <= m_nUsedHeight
	    && nScrollStart <  nScrollEnd)
	{
		m_nScrollStart = nScrollStart;
		m_nScrollEnd   = nScrollEnd;
	}

	CursorHome ();
}

// TODO: standout mode should be useable together with one other mode
void CTerminalDevice::SetStandoutMode (unsigned nMode)
{
	switch (nMode)
	{
	case 0:
	case 27:
		m_bReverseAttribute = FALSE;
		m_Color = m_pDisplay->GetColor (CDisplay::NormalColor);
		break;

	case 1:
		m_Color = m_pDisplay->GetColor (CDisplay::HighColor);
		break;

	case 2:
		m_Color = m_pDisplay->GetColor (CDisplay::HalfColor);
		break;

	case 7:
		m_bReverseAttribute = TRUE;
		break;

	case 30:
		m_Color = m_pDisplay->GetColor (CDisplay::Black);
		break;
	case 31:
		m_Color = m_pDisplay->GetColor (CDisplay::Red);
		break;
	case 32:
		m_Color = m_pDisplay->GetColor (CDisplay::Green);
		break;
	case 33:
		m_Color = m_pDisplay->GetColor (CDisplay::Yellow);
		break;
	case 34:
		m_Color = m_pDisplay->GetColor (CDisplay::Blue);
		break;
	case 35:
		m_Color = m_pDisplay->GetColor (CDisplay::Magenta);
		break;
	case 36:
		m_Color = m_pDisplay->GetColor (CDisplay::Cyan);
		break;
	case 37:
		m_Color = m_pDisplay->GetColor (CDisplay::White);
		break;

	case 40:
		m_BackgroundColor = m_pDisplay->GetColor (CDisplay::Black);
		break;
	case 41:
		m_BackgroundColor = m_pDisplay->GetColor (CDisplay::Red);
		break;
	case 42:
		m_BackgroundColor = m_pDisplay->GetColor (CDisplay::Green);
		break;
	case 43:
		m_BackgroundColor = m_pDisplay->GetColor (CDisplay::Yellow);
		break;
	case 44:
		m_BackgroundColor = m_pDisplay->GetColor (CDisplay::Blue);
		break;
	case 45:
		m_BackgroundColor = m_pDisplay->GetColor (CDisplay::Magenta);
		break;
	case 46:
		m_BackgroundColor = m_pDisplay->GetColor (CDisplay::Cyan);
		break;
	case 47:
		m_BackgroundColor = m_pDisplay->GetColor (CDisplay::White);
		break;

	case 90:
		m_Color = m_pDisplay->GetColor (CDisplay::BrightBlack);
		break;
	case 91:
		m_Color = m_pDisplay->GetColor (CDisplay::BrightRed);
		break;
	case 92:
		m_Color = m_pDisplay->GetColor (CDisplay::BrightGreen);
		break;
	case 93:
		m_Color = m_pDisplay->GetColor (CDisplay::BrightYellow);
		break;
	case 94:
		m_Color = m_pDisplay->GetColor (CDisplay::BrightBlue);
		break;
	case 95:
		m_Color = m_pDisplay->GetColor (CDisplay::BrightMagenta);
		break;
	case 96:
		m_Color = m_pDisplay->GetColor (CDisplay::BrightCyan);
		break;
	case 97:
		m_Color = m_pDisplay->GetColor (CDisplay::BrightWhite);
		break;

	case 100:
		m_BackgroundColor = m_pDisplay->GetColor (CDisplay::BrightBlack);
		break;
	case 101:
		m_BackgroundColor = m_pDisplay->GetColor (CDisplay::BrightRed);
		break;
	case 102:
		m_BackgroundColor = m_pDisplay->GetColor (CDisplay::BrightGreen);
		break;
	case 103:
		m_BackgroundColor = m_pDisplay->GetColor (CDisplay::BrightYellow);
		break;
	case 104:
		m_BackgroundColor = m_pDisplay->GetColor (CDisplay::BrightBlue);
		break;
	case 105:
		m_BackgroundColor = m_pDisplay->GetColor (CDisplay::BrightMagenta);
		break;
	case 106:
		m_BackgroundColor = m_pDisplay->GetColor (CDisplay::BrightCyan);
		break;
	case 107:
		m_BackgroundColor = m_pDisplay->GetColor (CDisplay::BrightWhite);
		break;
	default:
		break;
	}
}

void CTerminalDevice::Tabulator (void)
{
	unsigned nTabWidth = m_CharGen.GetCharWidth () * 8;

	m_nCursorX = ((m_nCursorX + nTabWidth) / nTabWidth) * nTabWidth;
	if (m_nCursorX >= m_nUsedWidth)
	{
		NewLine ();
	}
}

void CTerminalDevice::Scroll (void)
{
	unsigned nLines = m_CharGen.GetCharHeight ();

	u8 *pTo = m_pBuffer8 + m_nScrollStart * m_nPitch;
	u8 *pFrom = m_pBuffer8 + (m_nScrollStart + nLines) * m_nPitch;

	unsigned nSize = m_nPitch * (m_nScrollEnd - m_nScrollStart - nLines);
	if (nSize)
	{
		memcpy (pTo, pFrom, nSize);

		pTo += nSize;
	}

	nSize = m_nWidth * nLines;
	switch (m_nDepth)
	{
	case 1: {
			nSize /= 8;
			for (u8 *p = (u8 *) pTo; nSize--;)
			{
				*p++ = m_BackgroundColor ? 0xFF : 0;
			}
		}
		break;

	case 8: {
			for (u8 *p = (u8 *) pTo; nSize--;)
			{
				*p++ = (u8) m_BackgroundColor;
			}
		} break;

	case 16: {
			for (u16 *p = (u16 *) pTo; nSize--;)
			{
				*p++ = (u16) m_BackgroundColor;
			}
		} break;

	case 32: {
			for (u32 *p = (u32 *) pTo; nSize--;)
			{
				*p++ = (u32) m_BackgroundColor;
			}
		} break;
	}

	SetUpdateArea (0, m_nHeight-1);
}

void CTerminalDevice::DisplayChar (char chChar, unsigned nPosX, unsigned nPosY,
				   CDisplay::TRawColor nColor)
{
	for (unsigned y = 0; y < m_CharGen.GetCharHeight (); y++)
	{
		CCharGenerator::TPixelLine Line = m_CharGen.GetPixelLine (chChar, y);

		for (unsigned x = 0; x < m_CharGen.GetCharWidth (); x++)
		{
			SetRawPixel (nPosX + x, nPosY + y,   m_CharGen.GetPixel (x, Line)
							   ? nColor : GetTextBackgroundColor ());
		}
	}

	SetUpdateArea (nPosY, nPosY + m_CharGen.GetCharHeight ()-1);
}

void CTerminalDevice::EraseChar (unsigned nPosX, unsigned nPosY)
{
	for (unsigned y = 0; y < m_CharGen.GetCharHeight (); y++)
	{
		for (unsigned x = 0; x < m_CharGen.GetCharWidth (); x++)
		{
			SetRawPixel (nPosX + x, nPosY + y, m_BackgroundColor);
		}
	}

	SetUpdateArea (nPosY, nPosY + m_CharGen.GetCharHeight ()-1);
}

void CTerminalDevice::InvertCursor (void)
{
	if (!m_bCursorOn)
	{
		return;
	}

	CDisplay::TRawColor *pPixelData = m_pCursorPixels;
	unsigned y0 = m_bCursorBlock ? 0 : m_CharGen.GetUnderline ();
	for (unsigned y = y0; y < m_CharGen.GetCharHeight (); y++)
	{
		for (unsigned x = 0; x < m_CharGen.GetCharWidth (); x++)
		{
			if (!m_bCursorVisible)
			{
				// Store the old pixel
				*pPixelData++ = GetRawPixel (m_nCursorX + x, m_nCursorY + y);

				// Plot the cursor with the current FG Colour
				SetRawPixel (m_nCursorX + x, m_nCursorY + y, m_Color);
			}
			else
			{
				// Restore the backingstore for the cursor colour
				SetRawPixel (m_nCursorX + x, m_nCursorY + y, *pPixelData++);
			}
		}
	}

	m_bCursorVisible = !m_bCursorVisible;

	SetUpdateArea (m_nCursorY + y0, m_nCursorY + m_CharGen.GetCharHeight ()-1);
}
