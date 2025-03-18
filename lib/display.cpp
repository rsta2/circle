//
// display.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2024  R. Stange <rsta2@o2online.de>
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
#include <circle/display.h>
#include <circle/util.h>

CDisplay::CDisplay (TColorModel ColorModel)
:	m_ColorModel (ColorModel)
{
}

CDisplay::TColorModel CDisplay::GetColorModel (void) const
{
	return m_ColorModel;
}

CDisplay::TRawColor CDisplay::GetColor (TColor Color) const
{
	switch (m_ColorModel)
	{
	case RGB565:
		return    (Color & 0xF8) >> 3
			| (Color & 0xFC00) >> 5
			| (Color & 0xF80000) >> 8;

	case RGB565_BE:
		return bswap16 (  (Color & 0xF8) >> 3
				| (Color & 0xFC00) >> 5
				| (Color & 0xF80000) >> 8);

	case ARGB8888:
		return Color | 0xFF000000U;

	case I1:
		return !!Color;

	case I8:
		switch (Color)
		{
		case Black:		return 0;
		case Red:		return 1;
		case Green:		return 2;
		case Yellow:		return 3;
		case Blue:		return 4;
		case Magenta:		return 5;
		case Cyan:		return 6;
		case White:		return 7;
		case BrightBlack:	return 8;
		case BrightRed:		return 9;
		case BrightGreen:	return 10;
		case BrightYellow:	return 11;
		case BrightBlue:	return 12;
		case BrightMagenta:	return 13;
		case BrightCyan:	return 14;
		default:
		case BrightWhite:	return 15;
		}

	default:
		return 0;
	}
}

CDisplay::TColor CDisplay::GetColor (TRawColor nColor) const
{
	switch (m_ColorModel)
	{
	case RGB565_BE:
		nColor = bswap16 (nColor & 0xFFFF);
		// fall through

	case RGB565:
		switch (nColor & 0xFFFF)
		{
		default:
		case 0x0000:    return Black;
		case 0xA800:    return Red;
		case 0x0540:    return Green;
		case 0xAAA0:    return Yellow;
		case 0x0015:    return Blue;
		case 0xA815:    return Magenta;
		case 0x0555:    return Cyan;
		case 0xAD55:    return White;
		case 0x52AA:    return BrightBlack;
		case 0xFAAA:    return BrightRed;
		case 0x57EA:    return BrightGreen;
		case 0xFFEA:    return BrightYellow;
		case 0x52BF:    return BrightBlue;
		case 0xFABF:    return BrightMagenta;
		case 0x57FF:    return BrightCyan;
		case 0xFFFF:    return BrightWhite;
		}

	case ARGB8888:
		return (TColor) (nColor & 0xFFFFFF);

	case I1:
		return nColor ? BrightWhite : Black;

	case I8:
		switch (nColor)
		{
		default:
		case 0:		return Black;
		case 1:		return Red;
		case 2:		return Green;
		case 3:		return Yellow;
		case 4:		return Blue;
		case 5:		return Magenta;
		case 6:		return Cyan;
		case 7:		return White;
		case 8:		return BrightBlack;
		case 9:		return BrightRed;
		case 10:	return BrightGreen;
		case 11:	return BrightYellow;
		case 12:	return BrightBlue;
		case 13:	return BrightMagenta;
		case 14:	return BrightCyan;
		case 15:	return BrightWhite;
		}

	default:
		return Black;
	}
}

CDisplay *CDisplay::GetParent (void) const
{
	return nullptr;
}

unsigned CDisplay::GetOffsetX (void) const
{
	return 0;
}

unsigned CDisplay::GetOffsetY (void) const
{
	return 0;
}
