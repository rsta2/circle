//
// util.h
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
#ifndef _circle_util_h
#define _circle_util_h

#include <circle/macros.h>
#include <circle/types.h>

#ifdef __cplusplus
extern "C" {
#endif

void *memset (void *pBuffer, int nValue, size_t nLength);

void *memcpy (void *pDest, const void *pSrc, size_t nLength);
#define memcpyblk memcpy

void *memmove (void *pDest, const void *pSrc, size_t nLength);

int memcmp (const void *pBuffer1, const void *pBuffer2, size_t nLength);

size_t strlen (const char *pString);

int strcmp (const char *pString1, const char *pString2);
int strcasecmp (const char *pString1, const char *pString2);
int strncmp (const char *pString1, const char *pString2, size_t nMaxLen);
int strncasecmp (const char *pString1, const char *pString2, size_t nMaxLen);

char *strcpy (char *pDest, const char *pSrc);

char *strncpy (char *pDest, const char *pSrc, size_t nMaxLen);

char *strcat (char *pDest, const char *pSrc);

char *strchr (const char *pString, int chChar);
char *strstr (const char *pString, const char *pNeedle);

char *strtok_r (char *pString, const char *pDelim, char **ppSavePtr);

unsigned long strtoul (const char *pString, char **ppEndPtr, int nBase);
unsigned long long strtoull (const char *pString, char **ppEndPtr, int nBase);
int atoi (const char *pString);

int char2int (char chValue);			// with sign extension

#ifdef __GNUC__

#define bswap16		__builtin_bswap16
#define bswap32		__builtin_bswap32

#else

u16 bswap16 (u16 usValue);
u32 bswap32 (u32 ulValue);

#endif

#define le2be16		bswap16
#define le2be32		bswap32

#define be2le16		bswap16
#define be2le32		bswap32

#if !defined (__GNUC__) || (AARCH == 32 && STDLIB_SUPPORT == 0)
	int parity32 (unsigned nValue);		// returns number of ones % 1
#else
	#define parity32	__builtin_parity
#endif

#ifdef __cplusplus
}
#endif

#endif
