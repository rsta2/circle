//
/// \file terminal.h
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
#ifndef _circle_terminal_h
#define _circle_terminal_h

#include <circle/device.h>
#include <circle/display.h>
#include <circle/string.h>
#include <circle/chargenerator.h>
#include <circle/spinlock.h>
#include <circle/types.h>

#define TERMINAL_COLOR(red, green, blue)	DISPLAY_COLOR (read, green, blue)

typedef CDisplay::TColor TTerminalColor;

class CTerminalDevice : public CDevice	/// Terminal support for dot-matrix displays
{
public:
	/// \param pDisplay Pointer to display driver
	/// \param nDeviceIndex Instance number in device name service (0 -> "tty1");
	/// \param rFont Font to be used
	/// \param FontFlags Font flags
	CTerminalDevice (CDisplay *pDisplay, unsigned nDeviceIndex = 0,
			 const TFont &rFont = DEFAULT_FONT,
		         CCharGenerator::TFontFlags FontFlags = CCharGenerator::FontFlagsNone);

	~CTerminalDevice (void);

	/// \return Operation successful?
	boolean Initialize (void);

	/// \return Screen width in pixels
	unsigned GetWidth (void) const;
	/// \return Screen height in pixels
	unsigned GetHeight (void) const;

	/// \return Screen width in characters
	unsigned GetColumns (void) const;
	/// \return Screen height in characters
	unsigned GetRows (void) const;

	/// \return Pointer to display, we are working on
	CDisplay *GetDisplay (void);

	/// \brief Write characters to screen
	/// \note Supports several escape sequences (see: doc/screen.txt).
	/// \param pBuffer Pointer to the characters to be written
	/// \param nCount  Number of characters to be written
	/// \return Number of written characters
	int Write (const void *pBuffer, size_t nCount) override;

	/// \brief Set a pixel to a specific logical color
	/// \param nPosX X-Position of the pixel (based on 0)
	/// \param nPosY Y-Position of the pixel (based on 0)
	/// \param Color The logical color to be set
	void SetPixel (unsigned nPosX, unsigned nPosY, TTerminalColor Color);
	/// \brief Set a pixel to a specific raw color
	/// \param nPosX X-Position of the pixel (based on 0)
	/// \param nPosY Y-Position of the pixel (based on 0)
	/// \param nColor The raw color to be set
	void SetPixel (unsigned nPosX, unsigned nPosY, CDisplay::TRawColor nColor);
	/// \brief Get the color value of a pixel
	/// \param nPosX X-Position of the pixel (based on 0)
	/// \param nPosY Y-Position of the pixel (based on 0)
	/// \return The requested color value (CDisplay::Black if not matches)
	TTerminalColor GetPixel (unsigned nPosX, unsigned nPosY);

	/// \brief Enables a block cursor instead of the default underline
	void SetCursorBlock (boolean bCursorBlock);

	/// \brief Update the display from internal display buffer
	/// \param nMillis Update only, when N ms were passed since previous update (0 to disable)
	/// \note Once this method has been called, the display is only updated,
	///	  when this method is called and only in the given time interval.
	void Update (unsigned nMillis = 0);

public:
	/// \brief Set a pixel to a specific raw color
	/// \param nPosX X-Position of the pixel (based on 0)
	/// \param nPosY Y-Position of the pixel (based on 0)
	/// \param nColor The raw color to be set
	/// \note This method allows the direct access to the internal buffer.
	void SetRawPixel (unsigned nPosX, unsigned nPosY, CDisplay::TRawColor nColor)
	{
		switch (m_nDepth)
		{
		case 1: {
				u8 *pBuffer = &m_pBuffer8[(m_nWidth * nPosY + nPosX) / 8];
				u8 uchMask = 0x80 >> (nPosX & 7);
				if (nColor)
				{
					*pBuffer |= uchMask;
				}
				else
				{
					*pBuffer &= ~uchMask;
				}
			}
			break;

		case 8:		m_pBuffer8[m_nWidth * nPosY + nPosX] = (u8) nColor;	break;
		case 16:	m_pBuffer16[m_nWidth * nPosY + nPosX] = (u16) nColor;	break;
		case 32:	m_pBuffer32[m_nWidth * nPosY + nPosX] = nColor;		break;
		}
	}

	/// \brief Get the raw color value of a pixel
	/// \param nPosX X-Position of the pixel (based on 0)
	/// \param nPosY Y-Position of the pixel (based on 0)
	/// \return The requested raw color value
	/// \note This method allows the direct access to the internal buffer.
	CDisplay::TRawColor GetRawPixel (unsigned nPosX, unsigned nPosY)
	{
		switch (m_nDepth)
		{
		case 1: {
				u8 *pBuffer = &m_pBuffer8[(m_nWidth * nPosY + nPosX) / 8];
				u8 uchMask = 0x80 >> (nPosX & 7);
				return !!(*pBuffer & uchMask);
			}
			break;

		case 8:		return m_pBuffer8[m_nWidth * nPosY + nPosX];
		case 16:	return m_pBuffer16[m_nWidth * nPosY + nPosX];
		case 32:	return m_pBuffer32[m_nWidth * nPosY + nPosX];
		}

		return 0;
	}

private:
	void Write (char chChar);

	void CarriageReturn (void);
	void ClearDisplayEnd (void);
	void ClearLineEnd (void);
	void CursorDown (void);
	void CursorHome (void);
	void CursorLeft (void);
	void CursorMove (unsigned nRow, unsigned nColumn);
	void CursorRight (void);
	void CursorUp (void);
	void DeleteChars (unsigned nCount);
	void DeleteLines (unsigned nCount);
	void DisplayChar (char chChar);
	void EraseChars (unsigned nCount);
	CDisplay::TRawColor GetTextBackgroundColor (void);
	CDisplay::TRawColor GetTextColor (void);
	void InsertLines (unsigned nCount);
	void InsertMode (boolean bBegin);
	void NewLine (void);
	void ReverseScroll (void);
	void SetAutoPageMode (boolean bEnable);
	void SetCursorMode (boolean bVisible);
	void SetScrollRegion (unsigned nStartRow, unsigned nEndRow);
	void SetStandoutMode (unsigned nMode);
	void Tabulator (void);

	void Scroll (void);

	void DisplayChar (char chChar, unsigned nPosX, unsigned nPosY, CDisplay::TRawColor nColor);
	void EraseChar (unsigned nPosX, unsigned nPosY);
	void InvertCursor (void);

private:
	// We always update entire pixel lines.
	void SetUpdateArea (unsigned nPosY1, unsigned nPosY2)
	{
		if (nPosY1 < m_UpdateArea.y1)
		{
			m_UpdateArea.y1 = nPosY1;
		}

		if (nPosY2 > m_UpdateArea.y2)
		{
			m_UpdateArea.y2 = nPosY2;
		}
	}

private:
	enum TState
	{
		StateStart,
		StateEscape,
		StateBracket,
		StateNumber1,
		StateQuestionMark,
		StateSemicolon,
		StateNumber2,
		StateNumber3,
		StateAutoPage
	};

private:
	CDisplay	    *m_pDisplay;
	unsigned	     m_nDeviceIndex;
	CCharGenerator	     m_CharGen;
	CDisplay::TRawColor *m_pCursorPixels;
	union
	{
		u8	    *m_pBuffer8;
		u16	    *m_pBuffer16;
		u32	    *m_pBuffer32;
	};
	unsigned	     m_nSize;
	unsigned	     m_nPitch;
	unsigned	     m_nWidth;
	unsigned	     m_nHeight;
	unsigned	     m_nUsedWidth;
	unsigned	     m_nUsedHeight;
	unsigned	     m_nDepth;
	CDisplay::TArea	     m_UpdateArea;
	TState	 	     m_State;
	unsigned	     m_nScrollStart;
	unsigned	     m_nScrollEnd;
	unsigned	     m_nCursorX;
	unsigned	     m_nCursorY;
	boolean		     m_bCursorOn;
	boolean		     m_bCursorBlock;
	boolean		     m_bCursorVisible;
	CDisplay::TRawColor  m_Color;
	CDisplay::TRawColor  m_BackgroundColor;
	boolean		     m_bReverseAttribute;
	boolean		     m_bInsertOn;
	unsigned	     m_nParam1;
	unsigned	     m_nParam2;
	boolean		     m_bAutoPage;
	boolean		     m_bDelayedUpdate;
	unsigned	     m_nLastUpdateTicks;
	CSpinLock	     m_SpinLock;
};

#endif
