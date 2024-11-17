//
// st7789config.h
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
#ifndef _st7789config_h
#define _st7789config_h

// Configuration for a connected ST7789 SPI display
// See addon/display/sample/st7789/README for details!

#define SPI_MASTER_DEVICE	0		// 0, 4, 5, 6 on Raspberry Pi 4; 0 otherwise
#define SPI_CLOCK_SPEED		15000000	// Hz
#define SPI_CPOL		1		// try 0, if it does not work
#define SPI_CPHA		0		// try 1, if it does not work
#define SPI_CHIP_SELECT		0		// 0 or 1; don't care, if not connected

#define WIDTH			240		// display width in pixels
#define HEIGHT			240		// display height in pixels
#define DC_PIN			22
#define RESET_PIN		23		// or: CST7789Display::None
#define BACKLIGHT_PIN		CST7789Display::None

#endif
