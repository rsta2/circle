//
// chargenerator.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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
#include <circle/chargenerator.h>
#include <assert.h>
#include "font.h"

#undef GIMP_HEADER			// if font saved with GIMP with .h extension

#define FIRSTCHAR	0x21
#define LASTCHAR	0xFF
#define CHARCOUNT	(LASTCHAR - FIRSTCHAR + 1)

CCharGenerator::CCharGenerator (void)
#ifdef GIMP_HEADER
:	m_nCharWidth (width / CHARCOUNT)
#else
:	m_nCharWidth (width)
#endif
{
}

CCharGenerator::~CCharGenerator (void)
{
}

unsigned CCharGenerator::GetCharWidth (void) const
{
	return m_nCharWidth;
}

unsigned CCharGenerator::GetCharHeight (void) const
{
#ifdef GIMP_HEADER
	return height;
#else
	return height + extraheight;
#endif
}

unsigned CCharGenerator::GetUnderline (void) const
{
#ifdef GIMP_HEADER
	return height-3;
#else
	return height;
#endif
}

boolean CCharGenerator::GetPixel (char chAscii, unsigned nPosX, unsigned nPosY) const
{
	unsigned nAscii = (u8) chAscii;
	if (   nAscii < FIRSTCHAR
	    || nAscii > LASTCHAR)
	{
		return FALSE;
	}

	unsigned nIndex = nAscii - FIRSTCHAR;
	assert (nIndex < CHARCOUNT);

	assert (nPosX < m_nCharWidth);

#ifdef GIMP_HEADER
	assert (nPosY < height);
	unsigned nOffset = nPosY * width + nIndex * m_nCharWidth + nPosX;

	assert (nOffset < sizeof header_data / sizeof header_data[0]);
	return header_data[nOffset] ? TRUE : FALSE;
#else
	if (nPosY >= height)
	{
		return FALSE;
	}

	return font_data[nIndex][nPosY] & (0x80 >> nPosX) ? TRUE : FALSE;
#endif
}
