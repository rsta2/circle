//
// chargenerator.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2024  R. Stange <rsta2@o2online.de>
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

CCharGenerator::CCharGenerator (const TFont &rFont, TFontFlags Flags)
:	m_rFont (rFont),
	m_nWidthMult (Flags & FontFlagsDoubleWidth ? 2 : 1),
	m_nHeightMult (Flags & FontFlagsDoubleHeight ? 2 : 1),
	m_nCharWidth (rFont.width * m_nWidthMult),
	m_nCharWidthOrig (rFont.width),
	m_nCharHeight ((rFont.height + rFont.extra_height) * m_nHeightMult),
	m_nUnderline (rFont.height * m_nHeightMult)
{
}

CCharGenerator::TPixelLine CCharGenerator::GetPixelLine (char chAscii, unsigned nPosY) const
{
	unsigned nAscii = (u8) chAscii;
	if (   nAscii < m_rFont.first_char
	    || nAscii > m_rFont.last_char
	    || nPosY >= m_nUnderline)
	{
		return FALSE;
	}

	if (m_nHeightMult == 2)
	{
		nPosY >>= 1;
	}

	return m_rFont.data[(nAscii - m_rFont.first_char) * m_rFont.height + nPosY];
}
