//
// screen.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2023  R. Stange <rsta2@o2online.de>
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
#include <circle/screen.h>
#include <circle/devicenameservice.h>
#include <circle/synchronize.h>
#include <circle/util.h>

static const char DevicePrefix[] = "tty";

#define ROTORS		4

#ifndef SCREEN_HEADLESS

enum TScreenState
{
	ScreenStateStart,
	ScreenStateEscape,
	ScreenStateBracket,
	ScreenStateNumber1,
	ScreenStateQuestionMark,
	ScreenStateSemicolon,
	ScreenStateNumber2,
	ScreenStateNumber3
};

CScreenDevice::CScreenDevice (unsigned nWidth, unsigned nHeight, boolean bVirtual, unsigned nDisplay)
:	m_nInitWidth (nWidth),
	m_nInitHeight (nHeight),
	m_bVirtual (bVirtual),
	m_nDisplay (nDisplay),
	m_pFrameBuffer (0),
	m_pCursorPixels (0),
	m_pBuffer (0),
	m_nState (ScreenStateStart),
	m_nScrollStart (0),
	m_nCursorX (0),
	m_nCursorY (0),
	m_bCursorOn (TRUE),
	m_bCursorVisible (FALSE),
	m_Color (NORMAL_COLOR),
	m_BackgroundColor (BLACK_COLOR),
	m_ReverseAttribute (FALSE),
	m_bInsertOn (FALSE),
	m_bUpdated (FALSE)
#ifdef SCREEN_DMA_BURST_LENGTH
	, m_DMAChannel (DMA_CHANNEL_NORMAL)
#endif
#ifdef REALTIME
	, m_SpinLock (TASK_LEVEL)
#endif
{
}

CScreenDevice::~CScreenDevice (void)
{
	CDeviceNameService::Get ()->RemoveDevice (DevicePrefix, m_nDisplay+1, FALSE);

	if (m_bVirtual)
	{
		delete [] m_pBuffer;
	}
	m_pBuffer = 0;
	
	delete m_pFrameBuffer;
	m_pFrameBuffer = 0;

	delete [] m_pCursorPixels;
	m_pCursorPixels = 0;
}

boolean CScreenDevice::Initialize (void)
{
	if (!m_bVirtual)
	{
		m_pFrameBuffer = new CBcmFrameBuffer (m_nInitWidth, m_nInitHeight, DEPTH,
						      0, 0, m_nDisplay);
#if DEPTH == 8
		m_pFrameBuffer->SetPalette (RED_COLOR, RED_COLOR16);
		m_pFrameBuffer->SetPalette (GREEN_COLOR, GREEN_COLOR16);
		m_pFrameBuffer->SetPalette (YELLOW_COLOR, YELLOW_COLOR16);
		m_pFrameBuffer->SetPalette (BLUE_COLOR, BLUE_COLOR16);
		m_pFrameBuffer->SetPalette (MAGENTA_COLOR, MAGENTA_COLOR16);
		m_pFrameBuffer->SetPalette (CYAN_COLOR, CYAN_COLOR16);
		m_pFrameBuffer->SetPalette (WHITE_COLOR, WHITE_COLOR16);
		m_pFrameBuffer->SetPalette (BRIGHT_BLACK_COLOR, BRIGHT_BLACK_COLOR16);
		m_pFrameBuffer->SetPalette (BRIGHT_RED_COLOR, BRIGHT_RED_COLOR16);
		m_pFrameBuffer->SetPalette (BRIGHT_GREEN_COLOR, BRIGHT_GREEN_COLOR16);
		m_pFrameBuffer->SetPalette (BRIGHT_YELLOW_COLOR, BRIGHT_YELLOW_COLOR16);
		m_pFrameBuffer->SetPalette (BRIGHT_BLUE_COLOR, BRIGHT_BLUE_COLOR16);
		m_pFrameBuffer->SetPalette (BRIGHT_MAGENTA_COLOR, BRIGHT_MAGENTA_COLOR16);
		m_pFrameBuffer->SetPalette (BRIGHT_CYAN_COLOR, BRIGHT_CYAN_COLOR16);
		m_pFrameBuffer->SetPalette (BRIGHT_WHITE_COLOR, BRIGHT_WHITE_COLOR16);
#endif
		if (!m_pFrameBuffer->Initialize ())
		{
			return FALSE;
		}

		if (m_pFrameBuffer->GetDepth () != DEPTH)
		{
			return FALSE;
		}

		m_pBuffer = (TScreenColor *) (uintptr) m_pFrameBuffer->GetBuffer ();
		m_nSize   = m_pFrameBuffer->GetSize ();
		m_nPitch  = m_pFrameBuffer->GetPitch ();
		m_nWidth  = m_pFrameBuffer->GetWidth ();
		m_nHeight = m_pFrameBuffer->GetHeight ();

		m_pCursorPixels = new TScreenColor[
					m_CharGen.GetCharWidth () * 
				       (m_CharGen.GetCharHeight () - m_CharGen.GetUnderline ())];
		
		// Fail if we couldn't malloc the backing store for cursor pixels
		if (!m_pCursorPixels)
		{
			return FALSE;
		}
		
		// Ensure that each row is word-aligned so that we can safely use memcpyblk()
		if (m_nPitch % sizeof (u32) != 0)
		{
			return FALSE;
		}
		m_nPitch /= sizeof (TScreenColor);
	}
	else
	{
		m_nWidth = m_nInitWidth;
		m_nHeight = m_nInitHeight;
		m_nSize = m_nWidth * m_nHeight * sizeof (TScreenColor);
		m_nPitch = m_nWidth;

		m_pBuffer = new TScreenColor[m_nWidth * m_nHeight];
	}

	m_nUsedHeight = m_nHeight / m_CharGen.GetCharHeight () * m_CharGen.GetCharHeight ();
	m_nScrollEnd = m_nUsedHeight;

	CursorHome ();
	ClearDisplayEnd ();
	InvertCursor ();

	if (!CDeviceNameService::Get ()->GetDevice (DevicePrefix, m_nDisplay+1, FALSE))
	{
		CDeviceNameService::Get ()->AddDevice (DevicePrefix, m_nDisplay+1, this, FALSE);
	}

	return TRUE;
}

boolean CScreenDevice::Resize (unsigned nWidth, unsigned nHeight)
{
	if (m_bVirtual)
	{
		delete [] m_pBuffer;
	}
	m_pBuffer = 0;
	delete m_pFrameBuffer;
	m_pFrameBuffer = 0;
	delete [] m_pCursorPixels;
	m_pCursorPixels = 0;

	m_nInitWidth = nWidth;
	m_nInitHeight = nHeight;

	m_nState = ScreenStateStart;
	m_nScrollStart = 0;
	m_nCursorX = 0;
	m_nCursorY = 0;
	m_bCursorOn = TRUE;
	m_bCursorVisible = FALSE;
	m_Color = NORMAL_COLOR;
	m_BackgroundColor = BLACK_COLOR;
	m_ReverseAttribute = FALSE;
	m_bInsertOn = FALSE;
	m_bUpdated = FALSE;

	return Initialize ();
}

unsigned CScreenDevice::GetWidth (void) const
{
	return m_nWidth;
}

unsigned CScreenDevice::GetHeight (void) const
{
	return m_nHeight;
}

unsigned CScreenDevice::GetColumns (void) const
{
	return m_nWidth / m_CharGen.GetCharWidth ();
}

unsigned CScreenDevice::GetRows (void) const
{
	return m_nUsedHeight / m_CharGen.GetCharHeight ();
}

CBcmFrameBuffer *CScreenDevice::GetFrameBuffer (void)
{
	return m_pFrameBuffer;
}

TScreenStatus CScreenDevice::GetStatus (void)
{
	TScreenStatus Status;

	Status.pContent   = m_pBuffer;
	Status.nSize	  = m_nSize;
	Status.nState     = m_nState;
	Status.nScrollStart = m_nScrollStart;
	Status.nScrollEnd   = m_nScrollEnd;
	Status.nCursorX   = m_nCursorX;
	Status.nCursorY   = m_nCursorY;
	Status.bCursorOn  = m_bCursorOn;
	Status.Color      = m_Color;
	Status.BackgroundColor = m_BackgroundColor;
	Status.ReverseAttribute = m_ReverseAttribute;
	Status.bInsertOn  = m_bInsertOn;
	Status.nParam1    = m_nParam1;
	Status.nParam2    = m_nParam2;
	Status.bUpdated   = m_bUpdated;

	return Status;
}

boolean CScreenDevice::SetStatus (const TScreenStatus &Status)
{
	if (   m_nSize  != Status.nSize
	    || m_nPitch != m_nWidth)
	{
		return FALSE;
	}

	m_SpinLock.Acquire ();

	if (   m_bUpdated
	    || Status.bUpdated)
	{
		m_SpinLock.Release ();

		return FALSE;
	}

	memcpyblk (m_pBuffer, Status.pContent, m_nSize);

	m_nState     = Status.nState;
	m_nScrollStart = Status.nScrollStart;
	m_nScrollEnd   = Status.nScrollEnd;
	m_nCursorX   = Status.nCursorX;
	m_nCursorY   = Status.nCursorY;
	m_bCursorOn  = Status.bCursorOn;
	m_Color      = Status.Color;
	m_BackgroundColor = Status.BackgroundColor;
	m_ReverseAttribute = Status.ReverseAttribute;
	m_bInsertOn  = Status.bInsertOn;
	m_nParam1    = Status.nParam1;
	m_nParam2    = Status.nParam2;

	m_SpinLock.Release ();

	DataMemBarrier ();

	return TRUE;
}

int CScreenDevice::Write (const void *pBuffer, size_t nCount)
{
#ifdef REALTIME
	// cannot write from IRQ_LEVEL to prevent deadlock, just ignore it
	if (CurrentExecutionLevel () > TASK_LEVEL)
	{
		return nCount;
	}
#endif

	m_SpinLock.Acquire ();

	m_bUpdated = TRUE;
	
	InvertCursor ();
	
	const char *pChar = (const char *) pBuffer;
	int nResult = 0;

	while (nCount--)
	{
		Write (*pChar++);
		
		nResult++;
	}

	InvertCursor ();
	
	m_bUpdated = FALSE;

	m_SpinLock.Release ();

	DataMemBarrier ();

	return nResult;
}

void CScreenDevice::Write (char chChar)
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
		case 'M':
			ReverseScroll ();
			m_nState = ScreenStateStart;
			break;

		case '[':
			m_nState = ScreenStateBracket;
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

		case 'L':
			InsertLines (1);
			m_nState = ScreenStateStart;
			break;

		case 'M':
			DeleteLines (1);
			m_nState = ScreenStateStart;
			break;

		case 'P':
			DeleteChars (1);
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

		case 'L':
			InsertLines (m_nParam1);
			m_nState = ScreenStateStart;
			break;

		case 'M':
			DeleteLines (m_nParam1);
			m_nState = ScreenStateStart;
			break;

		case 'P':
			DeleteChars (m_nParam1);
			m_nState = ScreenStateStart;
			break;

		case 'X':
			EraseChars (m_nParam1);
			m_nState = ScreenStateStart;
			break;

		case 'h':
		case 'l':
			if (m_nParam1 == 4)
			{
				InsertMode (chChar == 'h');
			}
			m_nState = ScreenStateStart;
			break;
			
		case 'm':
			SetStandoutMode (m_nParam1);
			m_nState = ScreenStateStart;
			break;

		default:
			if ('0' <= chChar && chChar <= '9')
			{
				m_nParam1 *= 10;
				m_nParam1 += chChar - '0';

				if (m_nParam1 > 199)
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

		case 'r':
			SetScrollRegion (m_nParam1, m_nParam2);
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

	default:
		m_nState = ScreenStateStart;
		break;
	}
}

void CScreenDevice::CarriageReturn (void)
{
	m_nCursorX = 0;
}

void CScreenDevice::ClearDisplayEnd (void)
{
	ClearLineEnd ();
	
	unsigned nPosY = m_nCursorY + m_CharGen.GetCharHeight ();
	unsigned nOffset = nPosY * m_nPitch;
	
	TScreenColor *pBuffer = m_pBuffer + nOffset;
	unsigned nSize = m_nSize / sizeof (TScreenColor) - nOffset;
	
	while (nSize--)
	{
		*pBuffer++ = m_BackgroundColor;
	}
}

void CScreenDevice::ClearLineEnd (void)
{
	for (unsigned nPosX = m_nCursorX; nPosX < m_nWidth; nPosX += m_CharGen.GetCharWidth ())
	{
		EraseChar (nPosX, m_nCursorY);
	}
}

void CScreenDevice::CursorDown (void)
{
	m_nCursorY += m_CharGen.GetCharHeight ();
	if (m_nCursorY >= m_nScrollEnd)
	{
		Scroll ();

		m_nCursorY -= m_CharGen.GetCharHeight ();
	}
}

void CScreenDevice::CursorHome (void)
{
	m_nCursorX = 0;
	m_nCursorY = m_nScrollStart;
}

void CScreenDevice::CursorLeft (void)
{
	if (m_nCursorX > 0)
	{
		m_nCursorX -= m_CharGen.GetCharWidth ();
	}
	else
	{
		if (m_nCursorY > m_nScrollStart)
		{
			m_nCursorX = m_nWidth - m_CharGen.GetCharWidth ();
			m_nCursorY -= m_CharGen.GetCharHeight ();
		}
	}
}

void CScreenDevice::CursorMove (unsigned nRow, unsigned nColumn)
{
	unsigned nPosX = (nColumn - 1) * m_CharGen.GetCharWidth ();
	unsigned nPosY = (nRow - 1) * m_CharGen.GetCharHeight ();

	if (   nPosX < m_nWidth
	    && nPosY >= m_nScrollStart
	    && nPosY < m_nScrollEnd)
	{
		m_nCursorX = nPosX;
		m_nCursorY = nPosY;
	}
}

void CScreenDevice::CursorRight (void)
{
	m_nCursorX += m_CharGen.GetCharWidth ();
	if (m_nCursorX >= m_nWidth)
	{
		NewLine ();
	}
}

void CScreenDevice::CursorUp (void)
{
	if (m_nCursorY > m_nScrollStart)
	{
		m_nCursorY -= m_CharGen.GetCharHeight ();
	}
}

void CScreenDevice::DeleteChars (unsigned nCount)	// TODO
{
}

void CScreenDevice::DeleteLines (unsigned nCount)	// TODO
{
}

void CScreenDevice::DisplayChar (char chChar)
{
	// TODO: Insert mode
	
	if (' ' <= (unsigned char) chChar)
	{
		DisplayChar (chChar, m_nCursorX, m_nCursorY, GetTextColor ());

		CursorRight ();
	}
}

void CScreenDevice::EraseChars (unsigned nCount)
{
	if (nCount == 0)
	{
		return;
	}

	unsigned nEndX = m_nCursorX + nCount * m_CharGen.GetCharWidth ();
	if (nEndX > m_nWidth)
	{
		nEndX = m_nWidth;
	}

	for (unsigned nPosX = m_nCursorX; nPosX < nEndX; nPosX += m_CharGen.GetCharWidth ())
	{
		EraseChar (nPosX, m_nCursorY);
	}
}

TScreenColor CScreenDevice::GetTextBackgroundColor (void)
{
	return m_ReverseAttribute ? m_Color : m_BackgroundColor;
}

TScreenColor CScreenDevice::GetTextColor (void)
{
	return m_ReverseAttribute ? m_BackgroundColor : m_Color;
}

void CScreenDevice::InsertLines (unsigned nCount)	// TODO
{
}

void CScreenDevice::InsertMode (boolean bBegin)
{
	m_bInsertOn = bBegin;
}

void CScreenDevice::NewLine (void)
{
	CarriageReturn ();
	CursorDown ();
}

void CScreenDevice::ReverseScroll (void)
{
	if (   m_nCursorX == 0
	    && m_nCursorY == 0
	)
	{
		InsertLines (1);
	}
}

void CScreenDevice::SetCursorMode (boolean bVisible)
{
	m_bCursorOn = bVisible;
}

void CScreenDevice::SetScrollRegion (unsigned nStartRow, unsigned nEndRow)
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
void CScreenDevice::SetStandoutMode (unsigned nMode)
{
	switch (nMode)
	{
	case 0:
	case 27:
		m_ReverseAttribute = FALSE;
		m_Color = NORMAL_COLOR;
		break;
		
	case 1:
		m_Color = HIGH_COLOR;
		break;

	case 2:
		m_Color = HALF_COLOR;
		break;

	case 7:
		m_ReverseAttribute = TRUE;
		break;
		
	case 30:
		m_Color = BLACK_COLOR;
		break;
	case 31:
		m_Color = RED_COLOR;
		break;
	case 32:
		m_Color = GREEN_COLOR;
		break;
	case 33:
		m_Color = YELLOW_COLOR;
		break;
	case 34:
		m_Color = BLUE_COLOR;
		break;
	case 35:
		m_Color = MAGENTA_COLOR;
		break;
	case 36:
		m_Color = CYAN_COLOR;
		break;
	case 37:
		m_Color = WHITE_COLOR;
		break;

	case 40:
		m_BackgroundColor = BLACK_COLOR;
		break;
	case 41:
		m_BackgroundColor = RED_COLOR;
		break;
	case 42:
		m_BackgroundColor = GREEN_COLOR;
		break;
	case 43:
		m_BackgroundColor = YELLOW_COLOR;
		break;
	case 44:
		m_BackgroundColor = BLUE_COLOR;
		break;
	case 45:
		m_BackgroundColor = MAGENTA_COLOR;
		break;
	case 46:
		m_BackgroundColor = CYAN_COLOR;
		break;
	case 47:
		m_BackgroundColor = WHITE_COLOR;
		break;
		
	case 90:
		m_Color = BRIGHT_BLACK_COLOR;
		break;
	case 91:
		m_Color = BRIGHT_RED_COLOR;
		break;
	case 92:
		m_Color = BRIGHT_GREEN_COLOR;
		break;
	case 93:
		m_Color = BRIGHT_YELLOW_COLOR;
		break;
	case 94:
		m_Color = BRIGHT_BLUE_COLOR;
		break;
	case 95:
		m_Color = BRIGHT_MAGENTA_COLOR;
		break;
	case 96:
		m_Color = BRIGHT_CYAN_COLOR;
		break;
	case 97:
		m_Color = BRIGHT_WHITE_COLOR;
		break;
		
	case 100:
		m_BackgroundColor = BRIGHT_BLACK_COLOR;
		break;
	case 101:
		m_BackgroundColor = BRIGHT_RED_COLOR;
		break;
	case 102:
		m_BackgroundColor = BRIGHT_GREEN_COLOR;
		break;
	case 103:
		m_BackgroundColor = BRIGHT_YELLOW_COLOR;
		break;
	case 104:
		m_BackgroundColor = BRIGHT_BLUE_COLOR;
		break;
	case 105:
		m_BackgroundColor = BRIGHT_MAGENTA_COLOR;
		break;
	case 106:
		m_BackgroundColor = BRIGHT_CYAN_COLOR;
		break;
	case 107:
		m_BackgroundColor = BRIGHT_WHITE_COLOR;
		break;
	default:
		break;
	}
}

void CScreenDevice::Tabulator (void)
{
	unsigned nTabWidth = m_CharGen.GetCharWidth () * 8;
	
	m_nCursorX = ((m_nCursorX + nTabWidth) / nTabWidth) * nTabWidth;
	if (m_nCursorX >= m_nWidth)
	{
		NewLine ();
	}
}

void CScreenDevice::Scroll (void)
{
	unsigned nLines = m_CharGen.GetCharHeight ();

	u32 *pTo = (u32 *) (m_pBuffer + m_nScrollStart * m_nPitch);
	u32 *pFrom = (u32 *) (m_pBuffer + (m_nScrollStart + nLines) * m_nPitch);

	unsigned nSize = m_nPitch * (m_nScrollEnd - m_nScrollStart - nLines) * sizeof (TScreenColor);
	if (nSize > 0)
	{
#ifdef SCREEN_DMA_BURST_LENGTH
		m_DMAChannel.SetupMemCopy (pTo, pFrom, nSize, SCREEN_DMA_BURST_LENGTH, FALSE);

		m_DMAChannel.Start ();
		m_DMAChannel.Wait ();
#else
		unsigned nSizeBlk = nSize & ~0xF;
		memcpyblk (pTo, pFrom, nSizeBlk);

		// Handle framebuffers with row lengths not aligned to 16 bytes
		memcpy ((u8 *) pTo + nSizeBlk, (u8 *) pFrom + nSizeBlk, nSize & 0xF);
#endif

		pTo += nSize / sizeof (u32);
	}

	nSize = m_nPitch * nLines * sizeof (TScreenColor) / sizeof (u32);
	while (nSize--)
	{
		*pTo++ = m_BackgroundColor;
	}
}

void CScreenDevice::DisplayChar (char chChar, unsigned nPosX, unsigned nPosY, TScreenColor Color)
{
	for (unsigned y = 0; y < m_CharGen.GetCharHeight (); y++)
	{
		for (unsigned x = 0; x < m_CharGen.GetCharWidth (); x++)
		{
			SetPixel (nPosX + x, nPosY + y,  m_CharGen.GetPixel (chChar, x, y) ? 
								Color : GetTextBackgroundColor ());
		}
	}
}

void CScreenDevice::EraseChar (unsigned nPosX, unsigned nPosY)
{
	for (unsigned y = 0; y < m_CharGen.GetCharHeight (); y++)
	{
		for (unsigned x = 0; x < m_CharGen.GetCharWidth (); x++)
		{
			SetPixel (nPosX + x, nPosY + y, m_BackgroundColor);
		}
	}
}

void CScreenDevice::InvertCursor (void)
{
	if (!m_bCursorOn)
	{
		return;
	}

	TScreenColor *pPixelData = m_pCursorPixels;
	for (unsigned y = m_CharGen.GetUnderline (); y < m_CharGen.GetCharHeight (); y++)
	{
		for (unsigned x = 0; x < m_CharGen.GetCharWidth (); x++)
		{
			// Is the cursor currently visible?
			if (!m_bCursorVisible)
			{
				// Store the old pixel
				*pPixelData++ = GetPixel (m_nCursorX + x, m_nCursorY + y);

				// Plot the underscore with the current FG Colour
				SetPixel (m_nCursorX + x, m_nCursorY + y, m_Color);
			}
			else
			{
				// Restore the backinstore for the cursor colour
				SetPixel (m_nCursorX + x, m_nCursorY + y, *pPixelData++);
			}
		}
	}

	m_bCursorVisible = !m_bCursorVisible;
}

void CScreenDevice::SetPixel (unsigned nPosX, unsigned nPosY, TScreenColor Color)
{
	if (   nPosX < m_nWidth
	    && nPosY < m_nHeight)
	{
		m_pBuffer[m_nPitch * nPosY + nPosX] = Color;
	}
}

TScreenColor CScreenDevice::GetPixel (unsigned nPosX, unsigned nPosY)
{
	if (   nPosX < m_nWidth
	    && nPosY < m_nHeight)
	{
		return m_pBuffer[m_nPitch * nPosY + nPosX];
	}
	
	return m_BackgroundColor;
}

void CScreenDevice::Rotor (unsigned nIndex, unsigned nCount)
{
	static const char chChars[] = "-\\|/";

	nIndex %= ROTORS;
	nCount &= 4-1;

	unsigned nPosX = m_nWidth - (nIndex + 1) * m_CharGen.GetCharWidth ();

	DisplayChar (chChars[nCount], nPosX, 0, HIGH_COLOR);
}

#else	// #ifndef SCREEN_HEADLESS

CScreenDevice::CScreenDevice (unsigned nWidth, unsigned nHeight, boolean bVirtual, unsigned nDisplay)
:	m_nInitWidth (nWidth),
	m_nInitHeight (nHeight),
	m_nDisplay (nDisplay)
{
}

CScreenDevice::~CScreenDevice (void)
{
	CDeviceNameService::Get ()->RemoveDevice (DevicePrefix, m_nDisplay+1, FALSE);
}

boolean CScreenDevice::Initialize (void)
{
	if (   m_nInitWidth > 0
	    && m_nInitHeight > 0)
	{
		m_nWidth = m_nInitWidth;
		m_nHeight = m_nInitHeight;
	}
	else
	{
		m_nWidth = 640;
		m_nHeight = 480;
	}

	m_nUsedHeight = m_nHeight / m_CharGen.GetCharHeight () * m_CharGen.GetCharHeight ();

	if (!CDeviceNameService::Get ()->GetDevice (DevicePrefix, m_nDisplay+1, FALSE))
	{
		CDeviceNameService::Get ()->AddDevice (DevicePrefix, m_nDisplay+1, this, FALSE);
	}

	return TRUE;
}

boolean CScreenDevice::Resize (unsigned nWidth, unsigned nHeight)
{
	m_nInitWidth = nWidth;
	m_nInitHeight = nHeight;

	return Initialize ();
}

unsigned CScreenDevice::GetWidth (void) const
{
	return m_nWidth;
}

unsigned CScreenDevice::GetHeight (void) const
{
	return m_nHeight;
}

unsigned CScreenDevice::GetColumns (void) const
{
	return m_nWidth / m_CharGen.GetCharWidth ();
}

unsigned CScreenDevice::GetRows (void) const
{
	return m_nUsedHeight / m_CharGen.GetCharHeight ();
}

int CScreenDevice::Write (const void *pBuffer, size_t nCount)
{
	return nCount;
}

void CScreenDevice::SetPixel (unsigned nPosX, unsigned nPosY, TScreenColor Color)
{
}

TScreenColor CScreenDevice::GetPixel (unsigned nPosX, unsigned nPosY)
{
	return BLACK_COLOR;
}

void CScreenDevice::Rotor (unsigned nIndex, unsigned nCount)
{
}

#endif
