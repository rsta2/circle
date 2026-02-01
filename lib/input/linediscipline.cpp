//
// linediscipline.cpp
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
#include <circle/input/linediscipline.h>
#include <assert.h>

CLineDiscipline::CLineDiscipline (CDevice *pInputDevice, CDevice *pOutputDevice)
:	m_pInputDevice (pInputDevice),
	m_pOutputDevice (pOutputDevice),
	m_Mode (LineModeInput),
	m_bEcho (TRUE),
	m_pInPtr (m_Buffer),
	m_pInEnd (m_Buffer),
	m_bInsert (TRUE),
	m_pOutBufferPtr (m_OutBuffer),
	m_InputState (StateStart),
	m_nHistorySize (0),
	m_nHistoryIndex (0)
{
}

CLineDiscipline::~CLineDiscipline (void)
{
	m_pInputDevice = 0;
	m_pOutputDevice = 0;

	m_Mode = LineModeUnknown;
}

int CLineDiscipline::Read (void *pBuffer, size_t nCount)
{
	assert (pBuffer != 0);
	assert (nCount > 0);

	char *pchBuffer = (char *) pBuffer;
	int nResult = 0;

	while (1)
	{
		if (m_Mode == LineModeInput)
		{
			int nChar = GetChar ();
			if (nChar <= 0)
			{
				return nChar;
			}

			switch (nChar)
			{
			case '\b':
			case '\x7F':
				if (m_pInPtr > m_Buffer)	// if cursor not home
				{
					char *p = --m_pInPtr;	// to new cursor position
					PutChar ('\b');

					if (m_bInsert)
					{
						m_pInEnd--;	// shrink line

						// move characters left from cursor to end
						for (; p < m_pInEnd; p++)
						{
							PutChar (*p = *(p+1));
						}
					}

					// clear last character on device
					p++;
					PutChar (' ');

					// move cursor back to new position
					for (; p > m_pInPtr; p--)
					{
						PutChar ('\b');
					}

					Flush ();
				}
				break;

			case '\r':
			case '\n':
				// move cursor right to end of line, if necessary
				while (m_pInPtr < m_pInEnd)
				{
					PutChar (*m_pInPtr++);
				}

				// terminate and flush edit buffer
				*m_pInEnd++ = '\n';
				*m_pInEnd = '\0';
				PutChar ('\n');

				Flush ();

				AppendHistory ();

				// prepare for output
				m_pOutPtr = m_Buffer;
				m_Mode = LineModeOutput;
				break;

			case EscapeKeyDelete:
				if (m_pInPtr < m_pInEnd)	// if cursor not at end
				{
					m_pInEnd--;		// shrink buffer

					// move characters left from cursor to end
					char *p = m_pInPtr;
					for (; p < m_pInEnd; p++)
					{
						PutChar (*p = *(p+1));
					}

					// clear last character on device
					p++;
					PutChar (' ');

					// move cursor back to new position
					for (; p > m_pInPtr; p--)
					{
						PutChar ('\b');
					}

					Flush ();
				}
				break;

			case EscapeKeyLeft:
				if (m_pInPtr > m_Buffer)	// if cursor not home
				{
					m_pInPtr--;		// move cursor left
					PutChar ('\b');

					Flush ();
				}
				break;

			case EscapeKeyRight:
				if (m_pInPtr < m_pInEnd)	// if cursor not at end
				{
					PutChar (*m_pInPtr++);	// move cursor right

					Flush ();
				}
				break;

			case EscapeKeyHome:
				while (m_pInPtr > m_Buffer)	// while cursor not home
				{
					m_pInPtr--;		// move cursor left
					PutChar ('\b');
				}

				Flush ();
				break;

			case EscapeKeyEnd:
				while (m_pInPtr < m_pInEnd)	// while cursor not at end
				{
					PutChar (*m_pInPtr++);	// move cursor right
				}

				Flush ();
				break;

			case EscapeKeyInsert:
				m_bInsert = !m_bInsert;
				break;

			case EscapeKeyUp:
				MoveHistory (-1);
				break;

			case EscapeKeyDown:
				MoveHistory (1);
				break;

			default:
				if (' ' <= nChar && nChar <= EscapeKeyStart)	// printable char?
				{
					if (m_bInsert)
					{
						if (m_pInEnd < &m_Buffer[MaxLine]) // buffer not full
						{
							char *p = m_pInEnd++;	// to new end
							++m_pInPtr;		// cursor right

							// move chars right in buffer to new end
							for (; p >= m_pInPtr; p--)
							{
								*p = *(p-1);
							}

							*p = (char) nChar;	// insert new char

							// write from cursor to line end to device
							for (; p < m_pInEnd; p++)
							{
								PutChar (*p);
							}

							// move cursor back to position
							for (; p > m_pInPtr; p--)
							{
								PutChar ('\b');
							}

							Flush ();
						}
					}
					else
					{
						if (m_pInPtr < &m_Buffer[MaxLine]) // not at end
						{
							// insert new char and move cursor right
							*m_pInPtr++ = (char) nChar;
							PutChar (nChar);

							Flush ();

							// enlarge edit buffer, if beyond end
							if (m_pInPtr > m_pInEnd)
							{
								m_pInEnd = m_pInPtr;
							}

						}
					}
				}
				break;
			}
		}
		else if (m_Mode == LineModeOutput)
		{
			while (m_pOutPtr < m_pInEnd)
			{
				if (nCount == 0)
				{
					return nResult;
				}

				*pchBuffer++ = *m_pOutPtr++;
				nCount--;
				nResult++;
			}

			// prepare next input
			m_pInPtr = m_Buffer;
			m_pInEnd = m_Buffer;
			m_Mode = LineModeInput;
			m_bInsert = TRUE;

			if (nResult > 0)
			{
				return nResult;
			}
		}
		else if (m_Mode == LineModeRaw)
		{
			while (nCount > 0)
			{
				int nChar = GetChar ();
				if (nChar < 0)
				{
					Flush ();

					return nChar;
				}

				if (nChar == 0)
				{
					break;
				}

				PutChar (nChar);

				*pchBuffer++ = (char) nChar;
				nCount--;
				nResult++;
			}

			Flush ();

			return nResult;
		}
		else
		{
			assert (0);
		}
	}
}

void CLineDiscipline::SetOptionRawMode (boolean bEnable)
{
	m_Mode = bEnable ? LineModeRaw : LineModeInput;

	m_pInPtr = m_Buffer;
}

void CLineDiscipline::SetOptionEcho (boolean bEnable)
{
	m_bEcho = bEnable;
}

void CLineDiscipline::AppendHistory (void)
{
	// do append, if edit buffer is not empty and not equal to previous line
	if (   m_Buffer[0] != '\n'
	    && (   m_nHistorySize == 0
		|| m_History[m_nHistorySize-1].Compare (m_Buffer) != 0))
	{
		// scroll history up, if it is full
		if (m_nHistorySize == MaxHistorySize)
		{
			for (unsigned i = 0; i < MaxHistorySize-1; i++)
			{
				m_History[i] = m_History[i+1];
			}
		}
		else
		{
			m_nHistorySize++;	// otherwise increase history size
		}

		// set last history entry to edit buffer
		m_History[m_nHistorySize-1] = m_Buffer;
	}

	// set current index to behind last line in history
	m_nHistoryIndex = m_nHistorySize;
}

void CLineDiscipline::MoveHistory (int nBackFore)
{
	// do not move accross boundaries
	unsigned nNewIndex = m_nHistoryIndex + nBackFore;	// may wrap
	if (nNewIndex <= m_nHistorySize)
	{
		// clear line on device, if necessary
		const char *p = m_pInPtr;
		for (; p < m_pInEnd; p++)
		{
			PutChar (' ');
		}

		for (; p > m_Buffer; p--)
		{
			PutChar ('\b');
			PutChar (' ');
			PutChar ('\b');
		}

		Flush ();

		// terminate edit buffer
		*m_pInEnd++ = '\n';
		*m_pInEnd = '\0';

		// insert or append edit buffer to history
		if (m_nHistoryIndex < m_nHistorySize)
		{
			m_History[m_nHistoryIndex] = m_Buffer;
		}

		m_nHistoryIndex = nNewIndex;

		m_pInEnd = m_pInPtr = m_Buffer;		// clear edit buffer

		// if we are inside history,
		// set-up edit buffer and device with current line from history
		if (m_nHistoryIndex < m_nHistorySize)
		{
			for (const char *p = m_History[m_nHistoryIndex]; *p != '\n'; p++)
			{
				PutChar (*m_pInPtr++ = *p);

				m_pInEnd++;
			}

			Flush ();
		}
	}
}

int CLineDiscipline::GetChar (void)
{
	assert (m_pInputDevice != 0);

	unsigned char chChar;
	int nResult = m_pInputDevice->Read (&chChar, sizeof chChar);
	if (nResult > 0)
	{
		assert (nResult == sizeof chChar);
		nResult = chChar;

		if (m_Mode == LineModeRaw)
		{
			return nResult;
		}

		switch (m_InputState)
		{
		case StateStart:
			if (nResult == '\x1B')
			{
				nResult = 0;
				m_InputState = StateEscape;
			}
			break;

		case StateEscape:
			if (nResult == '[')
			{
				nResult = 0;
				m_InputState = StateSquareBracket1;
			}
			else
			{
				m_InputState = StateStart;
			}
			break;

		case StateSquareBracket1:
			m_InputState = StateStart;
			switch (nResult)
			{
			case 'A':
				nResult = EscapeKeyUp;
				break;

			case 'B':
				nResult = EscapeKeyDown;
				break;

			case 'C':
				nResult = EscapeKeyRight;
				break;

			case 'D':
				nResult = EscapeKeyLeft;
				break;

			case 'G':
				nResult = 0;
				break;

			case '[':
				nResult = 0;
				m_InputState = StateSquareBracket2;
				break;

			default:
				if ('0' <= nResult && nResult <= '9')
				{
					m_nInputParam = nResult - '0';
					nResult = 0;
					m_InputState = StateNumber1;
				}
				break;
			}
			break;

		case StateSquareBracket2:
			m_InputState = StateStart;
			if ('A' <= nResult && nResult <= 'E')
			{
				nResult = 0;
			}
			break;

		case StateNumber1:
			switch (nResult)
			{
			case ';':
				m_InputState = StateSemicolon;
				nResult = 0;
				break;

			case '~':
				m_InputState = StateStart;
				switch (m_nInputParam)
				{
				case 1:
					nResult = EscapeKeyHome;
					break;

				case 2:
					nResult = EscapeKeyInsert;
					break;

				case 3:
					nResult = EscapeKeyDelete;
					break;

				case 4:
					nResult = EscapeKeyEnd;
					break;

				default:
					nResult = 0;
					break;
				}
				break;

			default:
				if ('0' <= nResult && nResult <= '9')
				{
					m_nInputParam *= 10;
					m_nInputParam += nResult - '0';

					if (m_nInputParam > 20)
					{
						m_InputState = StateStart;
					}
				}
				break;
			}
			break;

		case StateSemicolon:
			if (nResult == '5')
			{
				nResult = 0;
				m_InputState = StateNumber2;
			}
			else
			{
				m_InputState = StateStart;
			}
			break;

		case StateNumber2:
			nResult = 0;
			m_InputState = StateStart;
			break;
		}
	}

	return nResult;
}

void CLineDiscipline::PutChar (int nChar)
{
	if (m_bEcho)
	{
		assert (m_pOutBufferPtr != 0);
		assert (m_pOutBufferPtr < &m_OutBuffer[OutBufferSize]);
		*m_pOutBufferPtr++ = (char) nChar;

		if (m_pOutBufferPtr == &m_OutBuffer[OutBufferSize])	// buffer full
		{
			Flush ();
		}
	}
}

void CLineDiscipline::Flush (void)
{
	assert (m_pOutBufferPtr != 0);
	size_t ulLen = m_pOutBufferPtr - m_OutBuffer;
	if (ulLen > 0)
	{
		assert (m_pOutputDevice != 0);
		m_pOutputDevice->Write (m_OutBuffer, ulLen);

		m_pOutBufferPtr = m_OutBuffer;
	}
}
