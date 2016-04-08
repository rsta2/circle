//
// string.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2016  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_string_h
#define _circle_string_h

#include <circle/stdarg.h>
#include <circle/types.h>

class CString
{
public:
	CString (void);
	CString (const char *pString);
	virtual ~CString (void);

	operator const char *(void) const;
	const char *operator = (const char *pString);
	
	size_t GetLength (void) const;

	void Append (const char *pString);
	int Compare (const char *pString) const;
	int Find (char chChar) const;			// returns index or -1 if not found

	void Format (const char *pFormat, ...);		// supports only a small subset of printf(3)
	void FormatV (const char *pFormat, va_list Args);

private:
	void PutChar (char chChar, size_t nCount = 1);
	void PutString (const char *pString);
	void ReserveSpace (size_t nSpace);
	
	static char *ntoa (char *pDest, unsigned long ulNumber, unsigned nBase, boolean bUpcase);
	static char *ftoa (char *pDest, double fNumber, unsigned nPrecision);

private:
	char 	 *m_pBuffer;
	unsigned  m_nSize;
	char	 *m_pInPtr;
};

#endif
