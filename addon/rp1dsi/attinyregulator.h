//
// attinyregulator.h
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
#ifndef _rp1dsi_attinyregulator_h
#define _rp1dsi_attinyregulator_h

#include <circle/i2cmaster.h>
#include <circle/genericlock.h>
#include <circle/types.h>

class CATTinyRegulator	/// Regulator driver for Raspberry Pi 7-inch touchscreen
{
public:
	enum gpio_signals
	{
		RST_BRIDGE_N,	/* TC358762 bridge reset */
		RST_TP_N,	/* Touch controller reset */
		NUM_GPIO
	};

public:
	CATTinyRegulator (CI2CMaster *pI2CMaster, u8 uchI2CAddress = 0x45);
	~CATTinyRegulator (void);

	boolean Initialize (void);

	// Voltage Regulator function
	boolean LCDPowerEnable (unsigned nRotation = 0);	// 0 or 180
	boolean LCDPowerDisable (void);
	boolean IsLCDPowerEnabled (void);

	// Backlight function
	boolean SetBacklight (unsigned nBrightness);		// 0 .. 255

	// GPIO function
	boolean GPIOWrite (unsigned nOffset, unsigned nValue);

private:
	boolean attiny_set_port_state (int reg, u8 val);
	u8 attiny_get_port_state (int reg);

	boolean regmap_write(u8 reg, u8 val);

	struct gpio_signal_mappings
	{
		unsigned reg;
		unsigned mask;
	};

private:
	CI2CMaster *m_pI2CMaster;
	u8 m_uchI2CAddress;

	u8 m_port_states[3];

	CGenericLock m_Lock;	/* lock to serialise overall accesses to the Atmel */

	static const gpio_signal_mappings s_mappings[NUM_GPIO];
};

#endif
