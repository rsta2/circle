// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 Marek Vasut <marex@denx.de>
 *
 * Based on tc358764.c by
 *  Andrzej Hajda <a.hajda@samsung.com>
 *  Maciej Purski <m.purski@samsung.com>
 *
 * Based on rpi_touchscreen.c by
 *  Eric Anholt <eric@anholt.net>
 *
 * Ported to Circle by Rene Stange
 */
#include "tc358762dsibridge.h"
#include "linuxcompat.h"
#include <circle/logger.h>
#include <circle/macros.h>
#include <assert.h>

/* PPI layer registers */
#define PPI_STARTPPI		0x0104 /* START control bit */
#define PPI_LPTXTIMECNT		0x0114 /* LPTX timing signal */
#define PPI_D0S_ATMR		0x0144
#define PPI_D1S_ATMR		0x0148
#define PPI_D0S_CLRSIPOCOUNT	0x0164 /* Assertion timer for Lane 0 */
#define PPI_D1S_CLRSIPOCOUNT	0x0168 /* Assertion timer for Lane 1 */
#define PPI_START_FUNCTION	1

/* DSI layer registers */
#define DSI_STARTDSI		0x0204 /* START control bit of DSI-TX */
#define DSI_LANEENABLE		0x0210 /* Enables each lane */
#define DSI_RX_START		1

/* LCDC/DPI Host Registers, based on guesswork that this matches TC358764 */
#define LCDCTRL			0x0420 /* Video Path Control */
#define LCDCTRL_MSF		BIT(0) /* Magic square in RGB666 */
#define LCDCTRL_VTGEN		BIT(4)/* Use chip clock for timing */
#define LCDCTRL_UNK6		BIT(6) /* Unknown */
#define LCDCTRL_EVTMODE		BIT(5) /* Event mode */
#define LCDCTRL_RGB888		BIT(8) /* RGB888 mode */
#define LCDCTRL_HSPOL		BIT(17) /* Polarity of HSYNC signal */
#define LCDCTRL_DEPOL		BIT(18) /* Polarity of DE signal */
#define LCDCTRL_VSPOL		BIT(19) /* Polarity of VSYNC signal */
#define LCDCTRL_VSDELAY(v)	(((v) & 0xfff) << 20) /* VSYNC delay */

/* First parameter is in the 16bits, second is in the top 16bits */
#define LCD_HS_HBP		0x0424
#define LCD_HDISP_HFP		0x0428
#define LCD_VS_VBP		0x042c
#define LCD_VDISP_VFP		0x0430

/* SPI Master Registers */
#define SPICMR			0x0450
#define SPITCR			0x0454

/* System Controller Registers */
#define SYSCTRL			0x0464

/* System registers */
#define LPX_PERIOD		3

/* Lane enable PPI and DSI register bits */
#define LANEENABLE_CLEN		BIT(0)
#define LANEENABLE_L0EN		BIT(1)
#define LANEENABLE_L1EN		BIT(2)

LOGMODULE ("tc358762");

CTC358762DSIBridge::CTC358762DSIBridge (mipi_dsi_device *pDevice,
					CATTinyRegulator *pRegulator,
					const drm_display_mode *pMode)
:	m_pDevice (pDevice),
	m_pRegulator (pRegulator),
	m_pMode (pMode),
	m_error (0)
{
}

CTC358762DSIBridge::~CTC358762DSIBridge (void)
{
	assert (m_pRegulator);
	m_pRegulator->GPIOWrite (CATTinyRegulator::RST_BRIDGE_N, 0);
}

boolean CTC358762DSIBridge::Initialize (void)
{
	assert (m_pRegulator);
	if (!m_pRegulator->GPIOWrite (CATTinyRegulator::RST_BRIDGE_N, 1))
	{
		LOGERR ("Cannot disable reset");

		return FALSE;
	}
	usleep_range(5000, 10000);

	int ret = tc358762_init ();
	if (ret < 0)
	{
		LOGERR ("Cannot init DSI bridge (%d)", ret);

		return FALSE;
	}

	return TRUE;
}

int CTC358762DSIBridge::tc358762_init (void)
{
	u32 lcdctrl;

	tc358762_write(DSI_LANEENABLE,
		       LANEENABLE_L0EN | LANEENABLE_CLEN);
	tc358762_write(PPI_D0S_CLRSIPOCOUNT, 5);
	tc358762_write(PPI_D1S_CLRSIPOCOUNT, 5);
	tc358762_write(PPI_D0S_ATMR, 0);
	tc358762_write(PPI_D1S_ATMR, 0);
	tc358762_write(PPI_LPTXTIMECNT, LPX_PERIOD);

	tc358762_write(SPICMR, 0x00);

	lcdctrl = LCDCTRL_VSDELAY(1) | LCDCTRL_RGB888 |
		  LCDCTRL_UNK6 | LCDCTRL_VTGEN;

	assert (m_pMode);
	if (m_pMode->flags & DRM_MODE_FLAG_NHSYNC)
		lcdctrl |= LCDCTRL_HSPOL;

	if (m_pMode->flags & DRM_MODE_FLAG_NVSYNC)
		lcdctrl |= LCDCTRL_VSPOL;

	tc358762_write(LCDCTRL, lcdctrl);

	tc358762_write(SYSCTRL, 0x040f);

	tc358762_write(LCD_HS_HBP, (m_pMode->hsync_end - m_pMode->hsync_start) |
		       ((m_pMode->htotal - m_pMode->hsync_end) << 16));
	tc358762_write(LCD_HDISP_HFP, m_pMode->hdisplay |
		       ((m_pMode->hsync_start - m_pMode->hdisplay) << 16));
	tc358762_write(LCD_VS_VBP, (m_pMode->vsync_end - m_pMode->vsync_start) |
		       ((m_pMode->vtotal - m_pMode->vsync_end) << 16));
	tc358762_write(LCD_VDISP_VFP, m_pMode->vdisplay |
		       ((m_pMode->vsync_start - m_pMode->vdisplay) << 16));
	msleep(100);

	tc358762_write(PPI_STARTPPI, PPI_START_FUNCTION);
	tc358762_write(DSI_STARTDSI, DSI_RX_START);

	msleep(100);

	return tc358762_clear_error();
}

void CTC358762DSIBridge::tc358762_write(u16 addr, u32 val)
{
	ssize_t ret;
	u8 data[6];

	if (m_error)
		return;

	data[0] = addr;
	data[1] = addr >> 8;
	data[2] = val;
	data[3] = val >> 8;
	data[4] = val >> 16;
	data[5] = val >> 24;

	assert (m_pDevice);
	ret = mipi_dsi_generic_write(m_pDevice, data, sizeof(data));
	if (ret < 0)
		m_error = ret;
}

int CTC358762DSIBridge::tc358762_clear_error (void)
{
	int ret = m_error;
	m_error = 0;

	return ret;
}
