//
// hd44780device.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2018  R. Stange <rsta2@o2online.de>
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
#ifndef _display_hd44780device_h
#define _display_hd44780device_h

#include <circle/device.h>
#include <circle/gpiopin.h>
#include <circle/spinlock.h>
#include <circle/types.h>

#define HD44780_MAX_COLUMNS	40
#define HD44780_MAX_ROWS	4

// ESCAPE SEQUENCES
//
// \E[B		Cursor down one line
// \E[H		Cursor home
// \E[A		Cursor up one line
// \E[%d;%dH	Cursor move to row %1 and column %2 (starting at 1)
// ^H		Cursor left one character
// \E[D		Cursor left one character
// \E[C		Cursor right one character
// ^M		Carriage return
// \E[J		Clear to end of screen
// \E[K		Clear to end of line
// \E[%dX	Erase %1 characters starting at cursor
// ^J		Carriage return/linefeed
// ^I		Move to next hardware tab
// \E[?25h	Normal cursor visible
// \E[?25l	Cursor invisible
//
// ^X = Control character
// \E = Escape (\x1b)
// %d = Numerical parameter (ASCII)

class CHD44780Device : public CDevice	/// LCD dot-matrix display driver (using HD44780 controller)
{
public:
	/// \param nColumns Display size in number of columns (max. 40)
	/// \param nRows    Display size in number of rows (max. 4)
	/// \param nD4Pin   GPIO pin number of Data 4 pin (Brcm numbering)
	/// \param nD5Pin   GPIO pin number of Data 5 pin (Brcm numbering)
	/// \param nD6Pin   GPIO pin number of Data 6 pin (Brcm numbering)
	/// \param nD7Pin   GPIO pin number of Data 7 pin (Brcm numbering)
	/// \param nENPin   GPIO pin number of Enable pin (Brcm numbering)
	/// \param nRSPin   GPIO pin number of Register Select pin (Brcm numbering)
	/// \param nRWPin   GPIO pin number of Read/Write pin (Brcm numbering, 0 if not connected)
	/// \note Driver uses 4-bit mode, pins D0-D3 are note used.
	CHD44780Device (unsigned nColumns, unsigned nRows,
			unsigned nD4Pin, unsigned nD5Pin, unsigned nD6Pin, unsigned nD7Pin,
			unsigned nENPin, unsigned nRSPin, unsigned nRWPin = 0);
	~CHD44780Device (void);

	/// \return Operation successful?
	boolean Initialize (void);

	/// \return Display size in number of columns
	unsigned GetColumns (void) const;
	/// \return Display size in number of rows
	unsigned GetRows (void) const;

	/// \brief Write characters to the display
	/// \note Supports several escape sequences (see above).
	/// \param pBuffer Pointer to the characters to be written
	/// \param nCount  Number of characters to be written
	/// \return Number of written characters
	int Write (const void *pBuffer, size_t nCount);

	/// \brief Define the 5x7 font for the definable characters
	/// \param chChar   Character code (0x80..0x87)
	/// \param FontData Font bit map for character pixel line 0-7 (only bits 4-0 are used)
	/// \note Line 7 is reserved for the cursor and is usually 0x00
	void DefineCharFont (char chChar, const u8 FontData[8]);

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
	void DisplayChar (char chChar);
	void EraseChars (unsigned nCount);
	void NewLine (void);
	void SetCursorMode (boolean bVisible);
	void Tabulator (void);

	void Scroll (void);

	void SetChar (unsigned nPosX, unsigned nPosY, char chChar);
	void SetCursor (void);

	void WriteByte (u8 nData);
	void WriteHalfByte (u8 nData);

	u8 GetDDRAMAddress (unsigned nPosX, unsigned nPosY);

private:
	unsigned m_nColumns;
	unsigned m_nRows;

	CGPIOPin  m_D4;
	CGPIOPin  m_D5;
	CGPIOPin  m_D6;
	CGPIOPin  m_D7;
	CGPIOPin  m_EN;
	CGPIOPin  m_RS;
	CGPIOPin *m_pRW;

	unsigned m_nState;
	unsigned m_nCursorX;
	unsigned m_nCursorY;
	boolean  m_bCursorOn;
	unsigned m_nParam1;
	unsigned m_nParam2;

	char m_Buffer[HD44780_MAX_ROWS][HD44780_MAX_COLUMNS];

	CSpinLock m_SpinLock;
};

#endif
