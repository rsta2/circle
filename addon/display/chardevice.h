//
// chardevice.h
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
#ifndef _display_chardevice_h
#define _display_chardevice_h

#include <circle/device.h>
#include <circle/i2cmaster.h>
#include <circle/spinlock.h>
#include <circle/types.h>

#define CHAR_MAX_COLUMNS	40
#define CHAR_MAX_ROWS	4

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
// \Ed+		Start autopage mode
// \Ed*		End autopage mode
//
// ^X = Control character
// \E = Escape (\x1b)
// %d = Numerical parameter (ASCII)

class CCharDevice : public CDevice	/// character based display driver
{
public:
	/// \param nColumns Display size in number of columns (max. 40)
	/// \param nRows    Display size in number of rows (max. 4)
	CCharDevice (unsigned nColumns, unsigned nRows);
	~CCharDevice (void);

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
	void SetAutoPageMode (boolean bEnable);
	void SetCursorMode (boolean bVisible);
	void Tabulator (void);

	void Scroll (void);
	void SetChar (unsigned nPosX, unsigned nPosY, char chChar);
	void SetCursor (void);

	/// Device specific routines to be provided by derived classes for each device
	virtual void DevClearCursor (void) = 0;
	virtual void DevSetCursor (unsigned nCursorX, unsigned nCursorY) = 0;
	virtual void DevSetCursorMode (boolean bVisible) = 0;
	virtual void DevSetChar (unsigned nPosX, unsigned nPosY, char chChar) = 0;
	virtual void DevUpdateDisplay (void) = 0;

private:
	unsigned m_nColumns;
	unsigned m_nRows;
	
	unsigned m_nState;
	unsigned m_nCursorX;
	unsigned m_nCursorY;
	boolean  m_bCursorOn;
	unsigned m_nParam1;
	unsigned m_nParam2;
	boolean  m_bAutoPage;

	char m_Buffer[CHAR_MAX_ROWS][CHAR_MAX_COLUMNS];

	CSpinLock m_SpinLock;
};

#endif
