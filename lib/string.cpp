//
// string.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
//
// ftoa() inspired by Arjan van Vught <info@raspberrypi-dmx.nl>
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
#include <circle/string.h>
#include <circle/util.h>

#define FORMAT_RESERVE		64	// additional bytes to allocate

#if AARCH == 32
	#define MAX_NUMBER_LEN		22	// 64 bit octal number
	#define MAX_PRECISION		9	// floor (log10 (ULONG_MAX))
#else
	#define MAX_NUMBER_LEN		22	// 64 bit octal number
	#define MAX_PRECISION		19	// floor (log10 (ULONG_MAX))
#endif

#define MAX_FLOAT_LEN		(1+MAX_NUMBER_LEN+1+MAX_PRECISION)

CString::CString (void)
:	m_pBuffer (0),
	m_nSize (0)
{
}

CString::CString (const char *pString)
{
	m_nSize = strlen (pString)+1;

	m_pBuffer = new char[m_nSize];

	strcpy (m_pBuffer, pString);
}

CString::CString (const CString &rString)
{
	m_nSize = strlen (rString)+1;

	m_pBuffer = new char[m_nSize];

	strcpy (m_pBuffer, rString);
}

CString::CString (CString &&rrString)
{
	m_nSize = rrString.m_nSize;
	m_pBuffer = rrString.m_pBuffer;

	rrString.m_nSize = 0;
	rrString.m_pBuffer = nullptr;
}

CString::~CString (void)
{
	delete [] m_pBuffer;
	m_pBuffer = 0;
}

CString::operator const char *(void) const
{
	if (m_pBuffer != 0)
	{
		return m_pBuffer;
	}

	return "";
}

const char *CString::operator = (const char *pString)
{
	delete [] m_pBuffer;

	m_nSize = strlen (pString)+1;

	m_pBuffer = new char[m_nSize];

	strcpy (m_pBuffer, pString);

	return m_pBuffer;
}

CString &CString::operator = (const CString &rString)
{
	delete [] m_pBuffer;

	m_nSize = strlen (rString)+1;

	m_pBuffer = new char[m_nSize];

	strcpy (m_pBuffer, rString);

	return *this;
}

CString &CString::operator = (CString &&rrString)
{
	delete [] m_pBuffer;

	m_nSize = rrString.m_nSize;
	m_pBuffer = rrString.m_pBuffer;

	rrString.m_nSize = 0;
	rrString.m_pBuffer = nullptr;

	return *this;
}

size_t CString::GetLength (void) const
{
	if (m_pBuffer == 0)
	{
		return 0;
	}
	
	return strlen (m_pBuffer);
}

void CString::Append (const char *pString)
{
	m_nSize = 1;		// for terminating '\0'
	if (m_pBuffer != 0)
	{
		m_nSize += strlen (m_pBuffer);
	}
	m_nSize += strlen (pString);

	char *pBuffer = new char[m_nSize];

	if (m_pBuffer != 0)
	{
		strcpy (pBuffer, m_pBuffer);
		delete [] m_pBuffer;
	}
	else
	{
		*pBuffer = '\0';
	}

	strcat (pBuffer, pString);

	m_pBuffer = pBuffer;
}

int CString::Compare (const char *pString) const
{
	return strcmp (m_pBuffer, pString);
}

int CString::Find (char chChar) const
{
	int nPos = 0;

	for (char *p = m_pBuffer; *p; p++)
	{
		if (*p == chChar)
		{
			return nPos;
		}

		nPos++;
	}

	return -1;
}

int CString::Replace (const char *pOld, const char *pNew)
{
	int nResult = 0;

	if (*pOld == '\0')
	{
		return nResult;
	}

	CString OldString (m_pBuffer);

	delete [] m_pBuffer;
	m_nSize = FORMAT_RESERVE;
	m_pBuffer = new char[m_nSize];
	m_pInPtr = m_pBuffer;

	const char *pReader = OldString.m_pBuffer;
	const char *pFound;
	while ((pFound = strchr (pReader, pOld[0])) != 0)
	{
		while (pReader < pFound)
		{
			PutChar (*pReader++);
		}

		const char *pPattern = pOld+1;
		const char *pCompare = pFound+1;
		while (   *pPattern != '\0'
		       && *pCompare == *pPattern)
		{
			pCompare++;
			pPattern++;
		}

		if (*pPattern == '\0')
		{
			PutString (pNew);
			pReader = pCompare;
			nResult++;
		}
		else
		{
			PutChar (*pReader++);
		}
	}

	PutString (pReader);

	*m_pInPtr = '\0';

	return nResult;
}

void CString::Format (const char *pFormat, ...)
{
	va_list var;
	va_start (var, pFormat);

	FormatV (pFormat, var);

	va_end (var);
}

void CString::FormatV (const char *pFormat, va_list Args)
{
	delete [] m_pBuffer;

	m_nSize = FORMAT_RESERVE;
	m_pBuffer = new char[m_nSize];
	m_pInPtr = m_pBuffer;

	while (*pFormat != '\0')
	{
		if (*pFormat == '%')
		{
			if (*++pFormat == '%')
			{
				PutChar ('%');
				
				pFormat++;

				continue;
			}

			boolean bAlternate = FALSE;
			if (*pFormat == '#')
			{
				bAlternate = TRUE;

				pFormat++;
			}

			boolean bLeft = FALSE;
			if (*pFormat == '-')
			{
				bLeft = TRUE;

				pFormat++;
			}

			boolean bNull = FALSE;
			if (*pFormat == '0')
			{
				bNull = TRUE;

				pFormat++;
			}

			size_t nWidth = 0;
			while ('0' <= *pFormat && *pFormat <= '9')
			{
				nWidth = nWidth * 10 + (*pFormat - '0');

				pFormat++;
			}

			unsigned nPrecision = 6;
			if (*pFormat == '.')
			{
				pFormat++;

				nPrecision = 0;
				while ('0' <= *pFormat && *pFormat <= '9')
				{
					nPrecision = nPrecision * 10 + (*pFormat - '0');

					pFormat++;
				}
			}

#if STDLIB_SUPPORT >= 1
			boolean bLong = FALSE;
			boolean bLongLong = FALSE;
			unsigned long long ullArg;
			long long llArg = 0;

			if (*pFormat == 'l')
			{
				if (*(pFormat+1) == 'l')
				{
					bLongLong = TRUE;

					pFormat++;
				}
				else
				{
					bLong = TRUE;
				}

				pFormat++;
			}
#else
			boolean bLong = FALSE;
			if (*pFormat == 'l')
			{
				bLong = TRUE;

				pFormat++;
			}
#endif
			char chArg;
			const char *pArg;
			unsigned long ulArg;
			size_t nLen;
			unsigned nBase;
			char NumBuf[MAX_FLOAT_LEN+1];
			boolean bMinus = FALSE;
			long lArg = 0;
			double fArg;

			switch (*pFormat)
			{
			case 'c':
				chArg = (char) va_arg (Args, int);
				if (bLeft)
				{
					PutChar (chArg);
					if (nWidth > 1)
					{
						PutChar (' ', nWidth-1);
					}
				}
				else
				{
					if (nWidth > 1)
					{
						PutChar (' ', nWidth-1);
					}
					PutChar (chArg);
				}
				break;

			case 'd':
			case 'i':
#if STDLIB_SUPPORT >= 1
				if (bLongLong)
				{
					llArg = va_arg (Args, long long);
				}
				else if (bLong)
#else
				if (bLong)
#endif
				{
					lArg = va_arg (Args, long);
				}
				else
				{
					lArg = va_arg (Args, int);
				}
				if (lArg < 0)
				{
					bMinus = TRUE;
					lArg = -lArg;
				}
#if STDLIB_SUPPORT >= 1
				if (llArg < 0)
				{
					bMinus = TRUE;
					llArg = -llArg;
				}
				if (bLongLong)
					lltoa (NumBuf, (unsigned long long) llArg, 10, FALSE);
				else
					ntoa (NumBuf, (unsigned long) lArg, 10, FALSE);
#else
				ntoa (NumBuf, (unsigned long) lArg, 10, FALSE);
#endif
				nLen = strlen (NumBuf) + (bMinus ? 1 : 0);
				if (bLeft)
				{
					if (bMinus)
					{
						PutChar ('-');
					}
					PutString (NumBuf);
					if (nWidth > nLen)
					{
						PutChar (' ', nWidth-nLen);
					}
				}
				else
				{
					if (!bNull)
					{
						if (nWidth > nLen)
						{
							PutChar (' ', nWidth-nLen);
						}
						if (bMinus)
						{
							PutChar ('-');
						}
					}
					else
					{
						if (bMinus)
						{
							PutChar ('-');
						}
						if (nWidth > nLen)
						{
							PutChar ('0', nWidth-nLen);
						}
					}
					PutString (NumBuf);
				}
				break;

			case 'f':
				fArg = va_arg (Args, double);
				ftoa (NumBuf, fArg, nPrecision);
				nLen = strlen (NumBuf);
				if (bLeft)
				{
					PutString (NumBuf);
					if (nWidth > nLen)
					{
						PutChar (' ', nWidth-nLen);
					}
				}
				else
				{
					if (nWidth > nLen)
					{
						PutChar (' ', nWidth-nLen);
					}
					PutString (NumBuf);
				}
				break;

			case 'o':
				if (bAlternate)
				{
					PutChar ('0');
				}
				nBase = 8;
				goto FormatNumber;

			case 's':
				pArg = va_arg (Args, const char *);
				nLen = strlen (pArg);
				if (bLeft)
				{
					PutString(pArg);
					if (nWidth > nLen)
					{
						PutChar (' ', nWidth-nLen);
					}
				}
				else
				{
					if (nWidth > nLen)
					{
						PutChar (' ', nWidth-nLen);
					}
					PutString (pArg);
				}
				break;

			case 'u':
				nBase = 10;
				goto FormatNumber;

			case 'x':
			case 'X':
			case 'p':
				if (bAlternate)
				{
					PutString (*pFormat == 'X' ? "0X" : "0x");
				}
				nBase = 16;
				goto FormatNumber;

			FormatNumber:
#if STDLIB_SUPPORT >= 1
				if (bLongLong)
				{
					ullArg = va_arg (Args, unsigned long long);
				}
				else if (bLong)
#else
				if (bLong)
#endif
				{
					ulArg = va_arg (Args, unsigned long);
				}
				else
				{
					ulArg = va_arg (Args, unsigned);
				}
#if STDLIB_SUPPORT >= 1
				if (bLongLong)
					lltoa (NumBuf, ullArg, nBase, *pFormat == 'X');
				else
					ntoa (NumBuf, ulArg, nBase, *pFormat == 'X');
#else
				ntoa (NumBuf, ulArg, nBase, *pFormat == 'X');
#endif
				nLen = strlen (NumBuf);
				if (bLeft)
				{
					PutString (NumBuf);
					if (nWidth > nLen)
					{
						PutChar (' ', nWidth-nLen);
					}
				}
				else
				{
					if (nWidth > nLen)
					{
						PutChar (bNull ? '0' : ' ', nWidth-nLen);
					}
					PutString (NumBuf);
				}
				break;

			default:
				PutChar ('%');
				PutChar (*pFormat);
				break;
			}
		}
		else
		{
			PutChar (*pFormat);
		}

		pFormat++;
	}

	*m_pInPtr = '\0';
}

void CString::PutChar (char chChar, size_t nCount)
{
	ReserveSpace (nCount);

	while (nCount--)
	{
		*m_pInPtr++ = chChar;
	}
}

void CString::PutString (const char *pString)
{
	size_t nLen = strlen (pString);
	
	ReserveSpace (nLen);
	
	strcpy (m_pInPtr, pString);
	
	m_pInPtr += nLen;
}

void CString::ReserveSpace (size_t nSpace)
{
	if (nSpace == 0)
	{
		return;
	}
	
	size_t nOffset = m_pInPtr - m_pBuffer;
	size_t nNewSize = nOffset + nSpace + 1;
	if (m_nSize >= nNewSize)
	{
		return;
	}
	
	nNewSize += FORMAT_RESERVE;
	char *pNewBuffer = new char[nNewSize];
		
	*m_pInPtr = '\0';
	strcpy (pNewBuffer, m_pBuffer);
	
	delete [] m_pBuffer;

	m_pBuffer = pNewBuffer;
	m_nSize = nNewSize;

	m_pInPtr = m_pBuffer + nOffset;
}

char *CString::ntoa (char *pDest, unsigned long ulNumber, unsigned nBase, boolean bUpcase)
{
	unsigned long ulDigit;

	unsigned long ulDivisor = 1UL;
	while (1)
	{
		ulDigit = ulNumber / ulDivisor;
		if (ulDigit < nBase)
		{
			break;
		}

		ulDivisor *= nBase;
	}

	char *p = pDest;
	while (1)
	{
		ulNumber %= ulDivisor;

		*p++ = ulDigit < 10 ? '0' + ulDigit : '0' + ulDigit + 7 + (bUpcase ? 0 : 0x20);

		ulDivisor /= nBase;
		if (ulDivisor == 0)
		{
			break;
		}

		ulDigit = ulNumber / ulDivisor;
	}

	*p = '\0';

	return pDest;
}

#if STDLIB_SUPPORT >= 1
char *CString::lltoa (char *pDest, unsigned long long ullNumber, unsigned nBase, boolean bUpcase)
{
	unsigned long long ullDigit;

	unsigned long long ullDivisor = 1ULL;
	while (1)
	{
		ullDigit = ullNumber / ullDivisor;
		if (ullDigit < nBase)
		{
			break;
		}

		ullDivisor *= nBase;
	}

	char *p = pDest;
	while (1)
	{
		ullNumber %= ullDivisor;

		*p++ = ullDigit < 10 ? '0' + ullDigit : '0' + ullDigit + 7 + (bUpcase ? 0 : 0x20);

		ullDivisor /= nBase;
		if (ullDivisor == 0)
		{
			break;
		}

		ullDigit = ullNumber / ullDivisor;
	}

	*p = '\0';

	return pDest;
}
#endif

char *CString::ftoa (char *pDest, double fNumber, unsigned nPrecision)
{
	char *p = pDest;

	if (fNumber < 0)
	{
		*p++ = '-';

		fNumber = -fNumber;
	}

	if (fNumber > (double) (unsigned long) -1)
	{
		strcpy (p, "overflow");

		return pDest;
	}

	unsigned long iPart = (unsigned long) fNumber;
	ntoa (p, iPart, 10, FALSE);

	if (nPrecision == 0)
	{
		return pDest;
	}

	p += strlen (p);
	*p++ = '.';

	if (nPrecision > MAX_PRECISION)
	{
		nPrecision = MAX_PRECISION;
	}

	unsigned long nPrecPow10 = 10;
	for (unsigned i = 2; i <= nPrecision; i++)
	{
		nPrecPow10 *= 10;
	}

	fNumber -= (double) iPart;
	fNumber *= (double) nPrecPow10;
	
	char Buffer[MAX_PRECISION+1];
	ntoa (Buffer, (unsigned long) fNumber, 10, FALSE);

	nPrecision -= strlen (Buffer);
	while (nPrecision--)
	{
		*p++ = '0';
	}

	strcpy (p, Buffer);
	
	return pDest;
}
