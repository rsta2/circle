// SPDX-License-Identifier: GPL-2.0
/*
 * Based on Linux driver:
 *	rpi-panel-v2-regulator.c
 *	by Dave Stevenson <dave.stevenson@raspberrypi.com>
 *	Copyright (C) 2022 Raspberry Pi Ltd.
 *
 * Based on rpi-panel-attiny-regulator.c by Marek Vasut <marex@denx.de>
 *
 * Ported to Circle by Rene Stange
 */
#include "rpipanel2regulator.h"
#include "linuxcompat.h"
#include <circle/logger.h>
#include <assert.h>

/* I2C registers of the microcontroller. */
#define REG_ID		0x01
#define REG_POWERON	0x02
#define REG_PWM		0x03

//bits for the PWM register
#define PWM_BL_ENABLE	0x80
#define PWM_VALUE	0x1F

LOGMODULE ("panel2");

CRPiPanel2Regulator::CRPiPanel2Regulator (CI2CMaster *pI2CMaster, u8 uchI2CAddress)
:	m_pI2CMaster (pI2CMaster),
	m_uchI2CAddress (uchI2CAddress),
	m_poweron_state (0)
{
}

CRPiPanel2Regulator::~CRPiPanel2Regulator (void)
{
	regmap_write (REG_PWM, 0);
	regmap_write (REG_POWERON, 0);
}

boolean CRPiPanel2Regulator::Initialize (void)
{
	u8 data;
	int ret = rpi_panel_v2_i2c_read (REG_ID, &data);
	if (ret < 0) {
		LOGERR ("Failed to read REG_ID reg: %d", ret);
		return FALSE;
	}

	switch (data & 0x0f) {
	case 0x01: /* 7 inch */
	case 0x04: /* 7 inch - old */
	case 0x08: /* 5 inch - old */
	case 0x09: /* 5 inch */
	case 0x0a: /* 10.1 inch */
		break;
	default:
		LOGERR ("Unknown revision: 0x%02x",
			data & 0x0f);
		return FALSE;
	}

	regmap_write(REG_POWERON, 0);

	return TRUE;
}

boolean CRPiPanel2Regulator::SetBacklight (unsigned nBrightness)
{
	assert (nBrightness <= PWM_VALUE);
	if (nBrightness)
	{
		nBrightness |= PWM_BL_ENABLE;
	}

	return regmap_write(REG_PWM, nBrightness);
}

boolean CRPiPanel2Regulator::GPIOWrite (unsigned off, unsigned val)
{
	u8 last_val;

	if (off >= NUM_GPIO)
		return FALSE;

	m_Lock.Acquire ();

	last_val = m_poweron_state;
	if (val)
		last_val |= (1 << off);
	else
		last_val &= ~(1 << off);

	m_poweron_state = last_val;

	boolean bResult = regmap_write(REG_POWERON, last_val);

	m_Lock.Release ();

	return bResult;
}

int CRPiPanel2Regulator::rpi_panel_v2_i2c_read (u8 reg, u8 *buf)
{
	/* Write register address */
	assert (m_pI2CMaster);
	int ret = m_pI2CMaster->Write (m_uchI2CAddress, &reg, 1);
	if (ret != 1)
		return ret;

	usleep_range(5000, 10000);

	/* Read data from register */
	ret = m_pI2CMaster->Read (m_uchI2CAddress, buf, 1);
	if (ret != 1)
		return ret;

	return 0;
}

boolean CRPiPanel2Regulator::regmap_write (u8 reg, u8 val)
{
	u8 buf[2] = {reg, val};

	assert (m_pI2CMaster);
	return m_pI2CMaster->Write (m_uchI2CAddress, buf, sizeof buf) == sizeof buf;
}
