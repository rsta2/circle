//
// linediscipline.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2018  R. Stange <rsta2@o2online.de>
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
	m_pInPtr (m_Buffer)
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
				if (m_pInPtr > m_Buffer)
				{
					PutChar ('\b');
					PutChar (' ');
					PutChar ('\b');

					m_pInPtr--;
				}
				break;

			case '\r':
			case '\n':
				*m_pInPtr++ = '\n';

				PutChar ('\n');

				m_pOutPtr = m_Buffer;
				m_Mode = LineModeOutput;
				break;

			default:
				if (nChar >= ' ')
				{
					if (m_pInPtr < &m_Buffer[LD_MAXLINE-1])
					{
						*m_pInPtr++ = (char) nChar;

						PutChar (nChar);
					}
				}
			}
		}
		else if (m_Mode == LineModeOutput)
		{
			while (m_pOutPtr < m_pInPtr)
			{
				if (nCount == 0)
				{
					return nResult;
				}

				*pchBuffer++ = *m_pOutPtr++;
				nCount--;
				nResult++;
			}

			m_pInPtr = m_Buffer;
			m_Mode = LineModeInput;

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

int CLineDiscipline::GetChar (void)
{
	assert (m_pInputDevice != 0);

	unsigned char chChar;
	int nResult = m_pInputDevice->Read (&chChar, sizeof chChar);
	if (nResult > 0)
	{
		assert (nResult == sizeof chChar);
		nResult = chChar;
	}

	return nResult;
}

void CLineDiscipline::PutChar (int nChar)
{
	if (m_bEcho)
	{
		assert (m_pOutputDevice != 0);

		char chChar = (char) nChar;
#ifndef NDEBUG
		int nResult =
#endif
			m_pOutputDevice->Write (&chChar, sizeof chChar);
		assert (nResult == sizeof chChar);
	}
}
