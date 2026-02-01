//
// linediscipline.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2025  R. Stange <rsta2@gmx.net>
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
#ifndef _circle_input_linediscipline_h
#define _circle_input_linediscipline_h

#include <circle/device.h>
#include <circle/string.h>
#include <circle/types.h>

class CLineDiscipline : public CDevice
{
public:
	CLineDiscipline (CDevice *pInputDevice, CDevice *pOutputDevice);
	~CLineDiscipline (void);

	int Read (void *pBuffer, size_t nCount);

	void SetOptionRawMode (boolean bEnable);
	void SetOptionEcho (boolean bEnable);

private:
	void AppendHistory (void);
	void MoveHistory (int nBackFore);

	enum TEscapeKey : int
	{
		EscapeKeyStart	= 256,
		EscapeKeyLeft	= EscapeKeyStart,
		EscapeKeyRight,
		EscapeKeyUp,
		EscapeKeyDown,
		EscapeKeyHome,
		EscapeKeyEnd,
		EscapeKeyInsert,
		EscapeKeyDelete
	};

	// read char from input device and detect escape keys, if not in raw mode
	int GetChar (void);

	void PutChar (int nChar);
	void Flush (void);		// write m_OutBuffer to output device

private:
	CDevice *m_pInputDevice;
	CDevice *m_pOutputDevice;

	enum TLineMode
	{
		LineModeInput,
		LineModeOutput,
		LineModeRaw,
		LineModeUnknown
	};

	TLineMode m_Mode;
	boolean m_bEcho;

	static const unsigned MaxLine = 160;
	char m_Buffer[MaxLine+2];	// edit buffer, will be terminated with "\n\0"

	char *m_pInPtr;			// cursor position in edit buffer
	char *m_pInEnd;			// current end of edit buffer
	boolean m_bInsert;

	char *m_pOutPtr;		// current position in edit buffer, while writing it out

	// collect characters here to write them out at once
	static const unsigned OutBufferSize = MaxLine*3;
	char m_OutBuffer[OutBufferSize];
	char *m_pOutBufferPtr;		// current position in m_OutBuffer

	enum TInputState
	{
		StateStart,
		StateEscape,		// '\x1B' read
		StateSquareBracket1,	// '[' read
		StateSquareBracket2,	// another '[' read
		StateNumber1,		// one or two decimal digits read
		StateSemicolon,		// ';' read
		StateNumber2		// '5' read
	};

	TInputState m_InputState;
	int m_nInputParam;

	static const unsigned MaxHistorySize = 40;
	CString m_History[MaxHistorySize];
	unsigned m_nHistorySize;
	unsigned m_nHistoryIndex;	// 0 .. m_nHistorySize
};

#endif
