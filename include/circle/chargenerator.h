//
// chargenerator.h
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
#ifndef _circle_chargenerator_h
#define _circle_chargenerator_h

#include <circle/font.h>
#include <circle/macros.h>
#include <circle/types.h>

class CCharGenerator	/// Gives pixel information for console font
{
public:
	enum TFontFlags : u8
	{
		FontFlagsNone		= 0,
		FontFlagsDoubleWidth	= BIT (0),
		FontFlagsDoubleHeight	= BIT (1),
		FontFlagsDoubleBoth	= FontFlagsDoubleWidth | FontFlagsDoubleHeight
	};

	typedef u8 TPixelLine;

public:
	/// \param rFont Font to be used
	/// \param Flags Font flags (can build with MakeFlags)
	CCharGenerator (const TFont &rFont = DEFAULT_FONT, TFontFlags Flags = FontFlagsNone);

	static TFontFlags MakeFlags (boolean bDoubleWidth, boolean bDoubleHeight)
	{
		return (TFontFlags) (  (bDoubleWidth ? FontFlagsDoubleWidth : 0)
				     | (bDoubleHeight ? FontFlagsDoubleHeight : 0));
	}

	/// \return Horizontal number of pixel per character
	unsigned GetCharWidth (void) const	{ return m_nCharWidth; }
	/// \return Vertical number of pixel per character (including underline space)
	unsigned GetCharHeight (void) const	{ return m_nCharHeight; }
	/// \return Vertical pixel start position of underline space
	unsigned GetUnderline (void) const	{ return m_nUnderline; }

	/// \param chAscii Character code (normally Latin1)
	/// \param nPosY Number of pixel line inside the character (0-based)
	/// \return Horizontal pixel information for the line
	TPixelLine GetPixelLine (char chAscii, unsigned nPosY) const;

	/// \param nPosX Horizontal pixel position inside the line (Left is 0)
	/// \param Line Horizontal pixel information for the line
	/// \return Is pixel is set?
	boolean GetPixel (unsigned nPosX, TPixelLine Line) const
	{
		if (m_nWidthMult == 2)
		{
			nPosX >>= 1;
		}

		return !!(Line & (1 << (m_nCharWidthOrig-1 - nPosX)));
	}

	/// \param chAscii Character code (normally Latin1)
	/// \param nPosX Horizontal pixel position inside the line (Left is 0)
	/// \param nPosY Number of pixel line inside the character (0-based)
	/// \return Is pixel is set?
	boolean GetPixel (char chAscii, unsigned nPosX, unsigned nPosY) const
	{
		return GetPixel (nPosX, GetPixelLine (chAscii, nPosY));
	}

private:
	const TFont &m_rFont;
	unsigned m_nWidthMult;
	unsigned m_nHeightMult;

	unsigned m_nCharWidth;
	unsigned m_nCharWidthOrig;
	unsigned m_nCharHeight;
	unsigned m_nUnderline;
};

#endif
