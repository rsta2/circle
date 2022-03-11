//
// util.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
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
#include <circle/util.h>

void *memmove (void *pDest, const void *pSrc, size_t nLength)
{
	char *pchDest = (char *) pDest;
	const char *pchSrc = (const char *) pSrc;

	if (   pchSrc < pchDest
	    && pchDest < pchSrc + nLength)
	{
		pchSrc += nLength;
		pchDest += nLength;

		while (nLength--)
		{
			*--pchDest = *--pchSrc;
		}

		return pDest;
	}

	return memcpy (pDest, pSrc, nLength);
}

#if STDLIB_SUPPORT <= 1

int memcmp (const void *pBuffer1, const void *pBuffer2, size_t nLength)
{
	const unsigned char *p1 = (const unsigned char *) pBuffer1;
	const unsigned char *p2 = (const unsigned char *) pBuffer2;
	
	while (nLength-- > 0)
	{
		if (*p1 > *p2)
		{
			return 1;
		}
		else if (*p1 < *p2)
		{
			return -1;
		}

		p1++;
		p2++;
	}

	return 0;
}

size_t strlen (const char *pString)
{
	size_t nResult = 0;

	while (*pString++)
	{
		nResult++;
	}

	return nResult;
}

int strcmp (const char *pString1, const char *pString2)
{
	while (   *pString1 != '\0'
	       && *pString2 != '\0')
	{
		if (*pString1 > *pString2)
		{
			return 1;
		}
		else if (*pString1 < *pString2)
		{
			return -1;
		}

		pString1++;
		pString2++;
	}

	if (*pString1 > *pString2)
	{
		return 1;
	}
	else if (*pString1 < *pString2)
	{
		return -1;
	}

	return 0;
}

static int toupper (int c)
{
	if ('a' <= c && c <= 'z')
	{
		c -= 'a' - 'A';
	}

	return c;
}

int strcasecmp (const char *pString1, const char *pString2)
{
	int nChar1, nChar2;

	while (   (nChar1 = toupper (*pString1)) != '\0'
	       && (nChar2 = toupper (*pString2)) != '\0')
	{
		if (nChar1 > nChar2)
		{
			return 1;
		}
		else if (nChar1 < nChar2)
		{
			return -1;
		}

		pString1++;
		pString2++;
	}

	nChar2 = toupper (*pString2);

	if (nChar1 > nChar2)
	{
		return 1;
	}
	else if (nChar1 < nChar2)
	{
		return -1;
	}

	return 0;
}

int strncmp (const char *pString1, const char *pString2, size_t nMaxLen)
{
	while (   nMaxLen > 0
	       && *pString1 != '\0'
	       && *pString2 != '\0')
	{
		if (*pString1 > *pString2)
		{
			return 1;
		}
		else if (*pString1 < *pString2)
		{
			return -1;
		}

		nMaxLen--;
		pString1++;
		pString2++;
	}

	if (nMaxLen == 0)
	{
		return 0;
	}

	if (*pString1 > *pString2)
	{
		return 1;
	}
	else if (*pString1 < *pString2)
	{
		return -1;
	}

	return 0;
}

int strncasecmp (const char *pString1, const char *pString2, size_t nMaxLen)
{
	int nChar1, nChar2;

	while (   nMaxLen > 0
	       && (nChar1 = toupper (*pString1)) != '\0'
	       && (nChar2 = toupper (*pString2)) != '\0')
	{
		if (nChar1 > nChar2)
		{
			return 1;
		}
		else if (nChar1 < nChar2)
		{
			return -1;
		}

		nMaxLen--;
		pString1++;
		pString2++;
	}

	nChar2 = toupper (*pString2);

	if (nMaxLen == 0)
	{
		return 0;
	}

	if (nChar1 > nChar2)
	{
		return 1;
	}
	else if (nChar1 < nChar2)
	{
		return -1;
	}

	return 0;
}

char *strcpy (char *pDest, const char *pSrc)
{
	char *p = pDest;

	while (*pSrc)
	{
		*p++ = *pSrc++;
	}

	*p = '\0';

	return pDest;
}

char *strncpy (char *pDest, const char *pSrc, size_t nMaxLen)
{
	char *pResult = pDest;

	while (nMaxLen > 0)
	{
		if (*pSrc == '\0')
		{
			break;
		}

		*pDest++ = *pSrc++;
		nMaxLen--;
	}

	if (nMaxLen > 0)
	{
		*pDest = '\0';
	}

	return pResult;
}

char *strcat (char *pDest, const char *pSrc)
{
	char *p = pDest;

	while (*p)
	{
		p++;
	}

	while (*pSrc)
	{
		*p++ = *pSrc++;
	}

	*p = '\0';

	return pDest;
}

char *strchr (const char *pString, int chChar)
{
	while (*pString)
	{
		if (*pString == chChar)
		{
			return (char *) pString;
		}

		pString++;
	}

	return 0;
}

char *strstr (const char *pString, const char *pNeedle)
{
	if (!*pString)
	{
		if (*pNeedle)
		{
			return 0;
		}

		return (char *) pString;
	}

	while (*pString)
	{
		size_t i = 0;

		while (1)
		{
			if (!pNeedle[i])
			{
				return (char *) pString;
			}

			if (pNeedle[i] != pString[i])
			{
				break;
			}

			i++;
		}

		pString++;
	}

	return 0;
}

char *strtok_r (char *pString, const char *pDelim, char **ppSavePtr)
{
	char *pToken;

	if (pString == 0)
	{
		pString = *ppSavePtr;
	}

	if (pString == 0)
	{
		return 0;
	}

	if (*pString == '\0')
	{
		*ppSavePtr = 0;

		return 0;
	}

	while (strchr (pDelim, *pString) != 0)
	{
		pString++;
	}

	if (*pString == '\0')
	{
		*ppSavePtr = 0;

		return 0;
	}

	pToken = pString;

	while (   *pString != '\0'
	       && strchr (pDelim, *pString) == 0)
	{
		pString++;
	}

	if (*pString != '\0')
	{
		*pString++ = '\0';
	}

	*ppSavePtr = pString;

	return pToken;
}

unsigned long strtoul (const char *pString, char **ppEndPtr, int nBase)
{
	unsigned long ulResult = 0;
	unsigned long ulPrevResult;
	int bMinus = 0;
	int bFirst = 1;

	if (ppEndPtr != 0)
	{
		*ppEndPtr = (char *) pString;
	}

	if (   nBase != 0
	    && (   nBase < 2
	        || nBase > 36))
	{
		return ulResult;
	}

	int c;
	while ((c = *pString) == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v')
	{
		pString++;
	}

	if (   *pString == '+'
	    || *pString == '-')
	{
		if (*pString++ == '-')
		{
			bMinus = 1;
		}
	}

	if (*pString == '0')
	{
		pString++;

		if (   *pString == 'x'
		    || *pString == 'X')
		{
			if (   nBase != 0
			    && nBase != 16)
			{
				return ulResult;
			}

			nBase = 16;

			pString++;
		}
		else
		{
			if (nBase == 0)
			{
				nBase =  8;
			}
		}
	}
	else
	{
		if (nBase == 0)
		{
			nBase = 10;
		}
	}

	while (1)
	{
		int c = *pString;

		if (c < '0')
		{
			break;
		}

		if ('a' <= c && c <= 'z')
		{
			c -= 'a' - 'A';
		}

		if (c >= 'A')
		{
			c -= 'A' - '9' - 1;
		}

		c -= '0';

		if (c >= nBase)
		{
			break;
		}

		ulPrevResult = ulResult;

		ulResult *= nBase;
		ulResult += c;

		if (ulResult < ulPrevResult)
		{
			ulResult = (unsigned long) -1;

			if (ppEndPtr != 0)
			{
				*ppEndPtr = (char *) pString;
			}

			return ulResult;
		}

		pString++;
		bFirst = 0;
	}	

	if (ppEndPtr != 0)
	{
		*ppEndPtr = (char *) pString;
	}

	if (bFirst)
	{
		return ulResult;
	}

	if (bMinus)
	{
		ulResult = -ulResult;
	}

	return ulResult;
}

unsigned long long int strtoull (const char *pString, char **ppEndPtr, int nBase)
{
	unsigned long long ullResult = 0;
	unsigned long long ullPrevResult;
	int bMinus = 0;
	int bFirst = 1;

	if (ppEndPtr != 0)
	{
		*ppEndPtr = (char *) pString;
	}

	if (   nBase != 0
	    && (   nBase < 2
	        || nBase > 36))
	{
		return ullResult;
	}

	int c;
	while ((c = *pString) == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v')
	{
		pString++;
	}

	if (   *pString == '+'
	    || *pString == '-')
	{
		if (*pString++ == '-')
		{
			bMinus = 1;
		}
	}

	if (*pString == '0')
	{
		pString++;

		if (   *pString == 'x'
		    || *pString == 'X')
		{
			if (   nBase != 0
			    && nBase != 16)
			{
				return ullResult;
			}

			nBase = 16;

			pString++;
		}
		else
		{
			if (nBase == 0)
			{
				nBase =  8;
			}
		}
	}
	else
	{
		if (nBase == 0)
		{
			nBase = 10;
		}
	}

	while (1)
	{
		int c = *pString;

		if (c < '0')
		{
			break;
		}

		if ('a' <= c && c <= 'z')
		{
			c -= 'a' - 'A';
		}

		if (c >= 'A')
		{
			c -= 'A' - '9' - 1;
		}

		c -= '0';

		if (c >= nBase)
		{
			break;
		}

		ullPrevResult = ullResult;

		ullResult *= nBase;
		ullResult += c;

		if (ullResult < ullPrevResult)
		{
			ullResult = (unsigned long) -1;

			if (ppEndPtr != 0)
			{
				*ppEndPtr = (char *) pString;
			}

			return ullResult;
		}

		pString++;
		bFirst = 0;
	}

	if (ppEndPtr != 0)
	{
		*ppEndPtr = (char *) pString;
	}

	if (bFirst)
	{
		return ullResult;
	}

	if (bMinus)
	{
		ullResult = -ullResult;
	}

	return ullResult;
}

int atoi (const char *pString)
{
	return (int) strtoul (pString, 0, 10);
}

#endif

int char2int (char chValue)
{
	int nResult = chValue;

	if (nResult > 0x7F)
	{
		nResult |= -0x100;
	}

	return nResult;
}

#ifndef __GNUC__

u16 bswap16 (u16 usValue)
{
	return    ((usValue & 0x00FF) << 8)
		| ((usValue & 0xFF00) >> 8);
}

u32 bswap32 (u32 ulValue)
{
	return    ((ulValue & 0x000000FF) << 24)
		| ((ulValue & 0x0000FF00) << 8)
		| ((ulValue & 0x00FF0000) >> 8)
		| ((ulValue & 0xFF000000) >> 24);
}

#endif

#if !defined (__GNUC__) || (AARCH == 32 && STDLIB_SUPPORT == 0)

int parity32 (unsigned nValue)
{
	int nResult = 0;

	while (nValue != 0)
	{
		nResult ^= (nValue & 1);
		nValue >>= 1;
	}

	return nResult;
}

#endif
