//
// font.h
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
#ifndef _circle_font_h
#define _circle_font_h

struct TFont
{
	unsigned width;
	unsigned height;
	unsigned extra_height;

	unsigned first_char;
	unsigned last_char;

	const unsigned char *data;
};

extern const TFont Font6x7;
extern const TFont Font8x8;
extern const TFont Font8x10;
extern const TFont Font8x12;
extern const TFont Font8x14;
extern const TFont Font8x16;

#ifndef DEFAULT_FONT
#define DEFAULT_FONT	Font8x16
#endif

#endif
