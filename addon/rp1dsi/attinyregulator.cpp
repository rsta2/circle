// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 Marek Vasut <marex@denx.de>
 *
 * Based on rpi_touchscreen.c by Eric Anholt <eric@anholt.net>
 *
 * Ported to Circle by Rene Stange
 */
#include "attinyregulator.h"
#include "linuxcompat.h"
#include <assert.h>

/* I2C registers of the Atmel microcontroller. */
#define REG_ID			0x80
#define REG_PORTA		0x81
#define REG_PORTB		0x82
#define REG_PORTC		0x83
#define REG_POWERON		0x85
#define REG_PWM			0x86
#define REG_ADDR_L		0x8c
#define REG_ADDR_H		0x8d
#define REG_WRITE_DATA_H	0x90
#define REG_WRITE_DATA_L	0x91

#define PA_LCD_DITHB		BIT(0)
#define PA_LCD_MODE		BIT(1)
#define PA_LCD_LR		BIT(2)
#define PA_LCD_UD		BIT(3)

#define PB_BRIDGE_PWRDNX_N	BIT(0)
#define PB_LCD_VCC_N		BIT(1)
#define PB_LCD_MAIN		BIT(7)

#define PC_LED_EN		BIT(0)
#define PC_RST_TP_N		BIT(1)
#define PC_RST_LCD_N		BIT(2)
#define PC_RST_BRIDGE_N		BIT(3)

const CATTinyRegulator::gpio_signal_mappings
	CATTinyRegulator::s_mappings[CATTinyRegulator::NUM_GPIO] =
{
	[RST_BRIDGE_N] = { REG_PORTC, PC_RST_BRIDGE_N | PC_RST_LCD_N  },
	[RST_TP_N] = { REG_PORTC, PC_RST_TP_N },
};

CATTinyRegulator::CATTinyRegulator (CI2CMaster *pI2CMaster, u8 uchI2CAddress)
:	m_pI2CMaster (pI2CMaster),
	m_uchI2CAddress (uchI2CAddress),
	m_port_states {0, 0, 0}
{
}

CATTinyRegulator::~CATTinyRegulator (void)
{
	regmap_write(REG_PWM, 0);
	regmap_write(REG_POWERON, 0);
}

boolean CATTinyRegulator::Initialize (void)
{
	if (!regmap_write(REG_POWERON, 0))
		return FALSE;
	msleep(30);
	if (!regmap_write(REG_PWM, 0))
		return FALSE;

	return TRUE;
}

boolean CATTinyRegulator::LCDPowerEnable (unsigned nRotation)
{
	m_Lock.Acquire ();

	/* Ensure bridge, and tp stay in reset */
	if (!attiny_set_port_state(REG_PORTC, 0))
	{
		m_Lock.Release ();
		return FALSE;
	}
	usleep_range(5000, 10000);

	// Default to the same orientation as the closed source firmware used for the panel.
	if (!attiny_set_port_state(REG_PORTA, !nRotation ? PA_LCD_LR : PA_LCD_UD))
	{
		m_Lock.Release ();
		return FALSE;
	}
	usleep_range(5000, 10000);
	/* Main regulator on, and power to the panel (LCD_VCC_N) */
	if (!attiny_set_port_state(REG_PORTB, PB_LCD_MAIN))
	{
		m_Lock.Release ();
		return FALSE;
	}
	usleep_range(5000, 10000);
	/* Bring controllers out of reset */
	if (!attiny_set_port_state(REG_PORTC, PC_LED_EN))
	{
		m_Lock.Release ();
		return FALSE;
	}

	m_Lock.Release ();

	msleep(80);

	return TRUE;
}

boolean CATTinyRegulator::LCDPowerDisable (void)
{
	m_Lock.Acquire ();

	if (!regmap_write (REG_PWM, 0))
	{
		m_Lock.Release ();
		return FALSE;
	}
	usleep_range(5000, 10000);

	if (!attiny_set_port_state(REG_PORTA, 0))
	{
		m_Lock.Release ();
		return FALSE;
	}
	usleep_range(5000, 10000);
	if (!attiny_set_port_state(REG_PORTB, PB_LCD_VCC_N))
	{
		m_Lock.Release ();
		return FALSE;
	}
	usleep_range(5000, 10000);
	if (!attiny_set_port_state(REG_PORTC, 0))
	{
		m_Lock.Release ();
		return FALSE;
	}
	msleep(30);

	m_Lock.Release ();

	return TRUE;
}

boolean CATTinyRegulator::IsLCDPowerEnabled (void)
{
	return !!(m_port_states[REG_PORTC - REG_PORTA] & PC_RST_BRIDGE_N);
}

boolean CATTinyRegulator::SetBacklight (unsigned nBrightness)
{
	m_Lock.Acquire ();

	for (int i = 0; i < 10; i++) {
		if (regmap_write (REG_PWM, (u8) nBrightness))
			break;
	}

	m_Lock.Release ();

	return TRUE;
}

boolean CATTinyRegulator::GPIOWrite (unsigned off, unsigned val)
{
	u8 last_val;

	if (off >= NUM_GPIO)
		return FALSE;

	m_Lock.Acquire ();

	last_val = attiny_get_port_state(s_mappings[off].reg);
	if (val)
		last_val |= s_mappings[off].mask;
	else
		last_val &= ~s_mappings[off].mask;

	if (!attiny_set_port_state(s_mappings[off].reg, last_val))
	{
		m_Lock.Release ();
		return FALSE;
	}

	if (off == RST_BRIDGE_N && val) {
		usleep_range(5000, 8000);
		if (!regmap_write(REG_ADDR_H, 0x04))
		{
			m_Lock.Release ();
			return FALSE;
		}
		usleep_range(5000, 8000);
		if (!regmap_write(REG_ADDR_L, 0x7c))
		{
			m_Lock.Release ();
			return FALSE;
		}
		usleep_range(5000, 8000);
		if (!regmap_write(REG_WRITE_DATA_H, 0x00))
		{
			m_Lock.Release ();
			return FALSE;
		}
		usleep_range(5000, 8000);
		if (!regmap_write(REG_WRITE_DATA_L, 0x00))
		{
			m_Lock.Release ();
			return FALSE;
		}

		msleep(100);
	}

	m_Lock.Release ();

	return TRUE;
}

boolean CATTinyRegulator::attiny_set_port_state(int reg, u8 val)
{
	m_port_states[reg - REG_PORTA] = val;

	return regmap_write (reg, val);
};

u8 CATTinyRegulator::attiny_get_port_state(int reg)
{
	return m_port_states[reg - REG_PORTA];
};

boolean CATTinyRegulator::regmap_write(u8 reg, u8 val)
{
	u8 buf[2] = {reg, val};

	assert (m_pI2CMaster);
	return m_pI2CMaster->Write (m_uchI2CAddress, buf, sizeof buf) == sizeof buf;
}
