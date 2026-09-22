//
// rpipanel2regulator.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2026  R. Stange <rsta2@gmx.net>
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
#ifndef _rp1dsi_rpipanel2regulator_h
#define _rp1dsi_rpipanel2regulator_h

#include <circle/i2cmaster.h>
#include <circle/genericlock.h>
#include <circle/types.h>

class CRPiPanel2Regulator	/// Regulator driver for Raspberry Pi 7-inch V2 touchscreen
{
public:
	enum gpio_signals
	{
		LCD_RESET,	/* DSI panel reset */
		CTP_RESET,	/* Touch controller reset */
		NUM_GPIO
	};

public:
	CRPiPanel2Regulator (CI2CMaster *pI2CMaster, u8 uchI2CAddress = 0x45);
	~CRPiPanel2Regulator (void);

	boolean Initialize (void);

	// Backlight function
	boolean SetBacklight (unsigned nBrightness);		// 0 .. 31

	// GPIO function
	boolean GPIOWrite (unsigned nOffset, unsigned nValue);

private:
	int rpi_panel_v2_i2c_read (u8 reg, u8 *buf);
	boolean regmap_write (u8 reg, u8 val);

private:
	CI2CMaster *m_pI2CMaster;
	u8 m_uchI2CAddress;

	u8 m_poweron_state;

	CGenericLock m_Lock;	/* lock to serialise overall accesses to the Atmel */
};

#endif
