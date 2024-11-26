//
// sampleconfig.h
//
// Display configuration for sample programs
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
#ifndef _display_sampleconfig_h
#define _display_sampleconfig_h

#define DISPLAY_TYPE_ST7789	1
#define DISPLAY_TYPE_ILI9341	2
#define DISPLAY_TYPE_SSD1306	3

//// Configuration for ST7789-based SPI displays ///////////////////////////////

// See addon/display/sample/st7789/README for details!

#if SPI_DISPLAY == DISPLAY_TYPE_ST7789

#define DISPLAY_ROTATION	0		// degrees counterclockwise; 0, 90, 180 or 270

#define SPI_MASTER_DEVICE	0		// 0, 4, 5, 6 on Raspberry Pi 4;
						// 0, 1, 3, 5 on Raspberry Pi 5; 0 otherwise
#define SPI_CLOCK_SPEED		15000000	// Hz
#define SPI_CPOL		1		// try 0, if it does not work
#define SPI_CPHA		0		// try 1, if it does not work
#define SPI_CHIP_SELECT		0		// 0 or 1; don't care, if not connected

#define WIDTH			240		// display width in pixels
#define HEIGHT			240		// display height in pixels
#define DC_PIN			22		// GPIO SoC numbers (not header positions!)
#define RESET_PIN		23		// or: CST7789Display::None
#define BACKLIGHT_PIN		CST7789Display::None

#define DISPLAY_CLASS		CST7789Display
#define DISPLAY_PARAMETERS	DC_PIN, RESET_PIN, BACKLIGHT_PIN, WIDTH, HEIGHT, \
				SPI_CPOL, SPI_CPHA, SPI_CLOCK_SPEED, SPI_CHIP_SELECT

#include <display/st7789display.h>

//// Configuration for ILI9341-based SPI displays //////////////////////////////

#elif SPI_DISPLAY == DISPLAY_TYPE_ILI9341

#define DISPLAY_ROTATION	0		// degrees counterclockwise; 0, 90, 180 or 270

#define SPI_MASTER_DEVICE	0		// 0, 4, 5, 6 on Raspberry Pi 4;
						// 0, 1, 3, 5 on Raspberry Pi 5; 0 otherwise
#define SPI_CLOCK_SPEED		15000000	// Hz
#define SPI_CPOL		0
#define SPI_CPHA		0
#define SPI_CHIP_SELECT		0		// 0 or 1

#define WIDTH			240		// display width in pixels at rotation 0
#define HEIGHT			320		// display height in pixels at rotation 0
#define DC_PIN			22		// GPIO SoC numbers (not header positions!)
#define RESET_PIN		23		// or: CILI9341Display::None
#define BACKLIGHT_PIN		24		// or: CILI9341Display::None

#define DISPLAY_CLASS		CILI9341Display
#define DISPLAY_PARAMETERS	DC_PIN, RESET_PIN, BACKLIGHT_PIN, WIDTH, HEIGHT, \
				SPI_CPOL, SPI_CPHA, SPI_CLOCK_SPEED, SPI_CHIP_SELECT

#include <display/ili9341display.h>

//// Configuration for SSD1306-based I2C displays //////////////////////////////

#elif I2C_DISPLAY == DISPLAY_TYPE_SSD1306

#define DISPLAY_ROTATION	0		// degrees; 0 or 180

#define I2C_MASTER_DEVICE	(CMachineInfo::Get ()->GetDevice (DeviceI2CMaster))
#define I2C_CLOCK_SPEED		0		// Hz; 0 for system default
#define I2C_SLAVE_ADDRESS	0x3C		// or 0x3D

#define WIDTH			128		// display width in pixels; fixed
#define HEIGHT			32		// display height in pixels; or 64

#define DISPLAY_CLASS		CSSD1306Display
#define DISPLAY_PARAMETERS	WIDTH, HEIGHT, I2C_SLAVE_ADDRESS, I2C_CLOCK_SPEED

#include <display/ssd1306display.h>

////////////////////////////////////////////////////////////////////////////////

#endif

#endif
