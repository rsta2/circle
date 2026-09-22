// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2012 Simon Budig, <simon.budig@kernelconcepts.de>
 * Daniel Wagener <daniel.wagener@kernelconcepts.de> (M09 firmware support)
 * Lothar Waﬂmann <LW@KARO-electronics.de> (DT support)
 * Dario Binacchi <dario.binacchi@amarulasolutions.com> (regmap support)
 *
 * Ported to Circle by Rene Stange
 */
/*
 * This is a driver for the EDT "Polytouch" family of touch controllers
 * based on the FocalTech FT5x06 line of chips.
 *
 * Development of this driver has been sponsored by Glyn:
 *    http://www.glyn.com/Products/Displays
 */
#include "rpitouchscreen1.h"
#include "linuxcompat.h"
#include <circle/logger.h>
#include <circle/util.h>
#include <assert.h>

#define EDT_NAME_LEN			23

#define RESET_DELAY_MS			300	// reset deassert to I2C

#define TOUCH_EVENT_DOWN		0x00
#define TOUCH_EVENT_UP			0x01
#define TOUCH_EVENT_ON			0x02
#define TOUCH_EVENT_RESERVED		0x03

const drm_display_mode CRPiTouchScreen1::s_raspberrypi_7inch_mode =
{
	.clock = 30000,
	.hdisplay = 800,
	.hsync_start = 800 + 131,
	.hsync_end = 800 + 131 + 2,
	.htotal = 800 + 131 + 2 + 45,
	.vdisplay = 480,
	.vsync_start = 480 + 7,
	.vsync_end = 480 + 7 + 2,
	.vtotal = 480 + 7 + 2 + 22,
	.flags = DRM_MODE_FLAG_NVSYNC | DRM_MODE_FLAG_NHSYNC,
};

LOGMODULE ("rp1touch1");

CRPiTouchScreen1::CRPiTouchScreen1 (CInterruptSystem *pInterrupt, CI2CMaster *pI2C,
				    unsigned nDepth, unsigned nDisplay,
				    boolean bEnableTouch)
:	mipi_dsi_device {&m_DSI, 0, MIPI_DSI_MODE_LPM},
	m_pInterrupt (pInterrupt),
	m_pI2C (pI2C),
	m_bEnableTouch (bEnableTouch),
	m_nRotation (0),
	m_bATTinyInitialized (FALSE),
	m_ATTiny (pI2C),
	m_DSI (pInterrupt, nDepth, nDisplay, 1),
	m_Bridge (this, &m_ATTiny, &s_raspberrypi_7inch_mode),
	m_init_td_status (-1),
	m_nKnownIDs (0),
	m_pInterface (nullptr)
{
}

CRPiTouchScreen1::~CRPiTouchScreen1 (void)
{
	delete m_pInterface;
	m_pInterface = nullptr;

	if (m_bATTinyInitialized)
	{
		m_ATTiny.GPIOWrite (CATTinyRegulator::RST_TP_N, 0);
		m_ATTiny.LCDPowerDisable ();
	}
}

boolean CRPiTouchScreen1::Initialize (void)
{
	if (!m_ATTiny.Initialize ())
	{
		LOGERR ("Cannot init AT Tiny regulator");
		return FALSE;
	}
	usleep_range(10, 100);

	if (!m_ATTiny.LCDPowerEnable (m_nRotation))
	{
		LOGERR ("Cannot power on LCD");
		return FALSE;
	}
	usleep_range(5000, 6000);

	m_bATTinyInitialized = TRUE;

	if (m_bEnableTouch)
	{
		if (!m_ATTiny.GPIOWrite (CATTinyRegulator::RST_TP_N, 1))
		{
			LOGERR ("Cannot disable touchscreen reset");
			return FALSE;
		}
		msleep(RESET_DELAY_MS);

		int ret = edt_ft5x06_ts_identify ();
		if (ret)
		{
			LOGERR ("Touchscreen probe failed (%d)", ret);
			return FALSE;
		}

		LOGNOTE ("Model is %s", m_model_name.c_str());

		if (m_version != GENERIC_FT)
		{
			LOGERR ("Model is not supported");
			return FALSE;
		}

		/*
		* Dummy read access. EP0700MLP1 returns bogus data on the first
		* register read access and ignores writes.
		*/
		int val;
		regmap_read(0x00, &val);
	}

	if (!m_DSI.Initialize (&s_raspberrypi_7inch_mode))
	{
		LOGERR ("Cannot init DSI host controller");
		return FALSE;
	}

	if (!m_Bridge.Initialize ())
	{
		LOGERR ("Cannot init DSI bridge");
		return FALSE;
	}

	return TRUE;
}

boolean CRPiTouchScreen1::Start (void *pFrameBuffer, unsigned nPitch)
{
	if (!m_DSI.Start (pFrameBuffer, nPitch))
	{
		return FALSE;
	}

	if (m_bEnableTouch)
	{
		m_pInterface = new CTouchScreenDevice (UpdateStub, this);
		assert (m_pInterface);
	}

	return TRUE;
}

unsigned CRPiTouchScreen1::GetWidth (void) const
{
	return Width;
}

unsigned CRPiTouchScreen1::GetHeight (void) const
{
	return Height;
}

boolean CRPiTouchScreen1::SetBacklightBrightness (unsigned nBrightness)
{
	return m_ATTiny.SetBacklight (nBrightness);
}

void CRPiTouchScreen1::RegisterVerticalSyncHandler (TVerticalSyncHandler *pHandler, void *pParam)
{
	m_DSI.RegisterVBlankHandler (pHandler, pParam);
}

int CRPiTouchScreen1::edt_ft5x06_ts_identify (void)
{
	u8 rdbuf[EDT_NAME_LEN];
	char *p;
	int error;

	/* see what we find if we assume it is a M06 *
	 * if we get less than EDT_NAME_LEN, we don't want
	 * to have garbage in there
	 */
	memset(rdbuf, 0, sizeof(rdbuf));
	error = regmap_bulk_read(0xBB, rdbuf, EDT_NAME_LEN - 1);
	if (error)
		return error;

	/* Probe content for something consistent.
	 * M06 starts with a response byte, M12 gives the data directly.
	 * M09/Generic does not provide model number information.
	 */
	if (!strncasecmp((char *) rdbuf + 1, "EP0", 3)) {
		m_version = EDT_M06;

		/* remove last '$' end marker */
		rdbuf[EDT_NAME_LEN - 1] = '\0';
		if (rdbuf[EDT_NAME_LEN - 2] == '$')
			rdbuf[EDT_NAME_LEN - 2] = '\0';

		/* look for Model/Version separator */
		p = strchr((char *) rdbuf, '*');
		if (p)
			*p++ = '\0';
		m_model_name = (char *) rdbuf;
	} else if (!strncasecmp((char *) rdbuf, "EP0", 3)) {
		m_version = EDT_M12;

		/* remove last '$' end marker */
		rdbuf[EDT_NAME_LEN - 2] = '\0';
		if (rdbuf[EDT_NAME_LEN - 3] == '$')
			rdbuf[EDT_NAME_LEN - 3] = '\0';

		/* look for Model/Version separator */
		p = strchr((char *) rdbuf, '*');
		if (p)
			*p++ = '\0';
		m_model_name = (char *) rdbuf;
	} else {
		/* If it is not an EDT M06/M12 touchscreen, then the model
		 * detection is a bit hairy. The different ft5x06
		 * firmwares around don't reliably implement the
		 * identification registers. Well, we'll take a shot.
		 *
		 * The main difference between generic focaltec based
		 * touches and EDT M09 is that we know how to retrieve
		 * the max coordinates for the latter.
		 */
		m_version = GENERIC_FT;

		error = regmap_bulk_read(0xA8, rdbuf, 1);
		if (error)
			return error;

		/* This "model identification" is not exact. Unfortunately
		 * not all firmwares for the ft5x06 put useful values in
		 * the identification registers.
		 */
		switch (rdbuf[0]) {
		case 0x11:   /* EDT EP0110M09 */
		case 0x35:   /* EDT EP0350M09 */
		case 0x43:   /* EDT EP0430M09 */
		case 0x50:   /* EDT EP0500M09 */
		case 0x57:   /* EDT EP0570M09 */
		case 0x70:   /* EDT EP0700M09 */
			m_version = EDT_M09;
			m_model_name.Format ("EP0%d%d0M09", (unsigned) rdbuf[0] >> 4,
							    (unsigned) rdbuf[0] & 0x0F);
			break;
		case 0xa1:   /* EDT EP1010ML00 */
			m_version = EDT_M09;
			m_model_name.Format ("EP%d%d0ML00", (unsigned) rdbuf[0] >> 4,
							    (unsigned) rdbuf[0] & 0x0F);
			break;
		case 0x5a:   /* Solomon Goldentek Display */
			m_model_name = "GKTW50SCED1R0";
			break;
		case 0x59:  /* Evervision Display with FT5xx6 TS */
			m_version = EV_FT;
			m_model_name = "EVERVISION-FT5726NEi";
			break;
		default:
			m_model_name.Format ("generic FT5x06 (%02x)", (unsigned) rdbuf[0]);
			break;
		}
	}

	return 0;
}

static inline u16 get_unaligned_be16 (const void *p)
{
	return be2le16 (*static_cast<const u16 *> (p));
}

void CRPiTouchScreen1::edt_ft5x06_ts_isr (void)
{
	assert (m_pInterface);

	u8 rdbuf[63];
	memset(rdbuf, 0, sizeof(rdbuf));
	int error = regmap_bulk_read(tdata_cmd, rdbuf, tdata_len);

	/* Register 2 is TD_STATUS, containing the number of touch
	 * points.
	 */
	int num_points = min(rdbuf[2] & 0xf, MaxTouchPoints);

	/* When polling FT5x06 without IRQ: initial register contents
         * could be stale or undefined; discard all readings until
	 * TD_STATUS changes for the first time (or num_points is 0).
	 */
	if (m_init_td_status) {
		if (m_init_td_status < 0)
			m_init_td_status = rdbuf[2];

		if (num_points && rdbuf[2] == m_init_td_status)
			return;

		m_init_td_status = 0;
	}

	if (!error && num_points)
		error = regmap_bulk_read(tdata_offset,
					 &rdbuf[tdata_offset],
					 point_len * num_points);

	if (error) {
		static bool shown = false;
		if (!shown) {
			LOGWARN ("Unable to fetch data, error: %d", error);
			shown = true;
		}
		return;
	}

	unsigned active_ids = 0;
	unsigned type, x, y, id;

	for (int i = 0; i < num_points; i++)
	{
		u8 *buf = &rdbuf[i * point_len + tdata_offset];

		type = buf[0] >> 6;
		/* ignore Reserved events */
		if (type == TOUCH_EVENT_RESERVED)
			continue;

		x = get_unaligned_be16(buf) & 0x0fff;
		y = get_unaligned_be16(buf + 2) & 0x0fff;
		/* The FT5x26 send the y coordinate first */
		if (m_version == EV_FT)
		{
			int t = x;
			x = y;
			y = t;
		}

		if (!m_nRotation)
		{
			x = Width-1 - x;
			y = Height-1 - y;
		}
		x = min(x, Width-1);
		y = min(y, Height-1);

		id = (buf[2] >> 4) & 0x0f;

		active_ids |= BIT(id);

		if (type != TOUCH_EVENT_UP)
		{
			if (!(m_nKnownIDs & BIT(id)))
			{
				m_nPosX[id] = x;
				m_nPosY[id] = y;
				m_pInterface->ReportHandler (TouchScreenEventFingerDown, id, x, y);
			}
			else if (x != m_nPosX[id] || y != m_nPosY[id])
			{
				m_nPosX[id] = x;
				m_nPosY[id] = y;
				m_pInterface->ReportHandler (TouchScreenEventFingerMove, id, x, y);
			}
		}
	}

	unsigned released_ids = m_nKnownIDs & ~active_ids;
	for (unsigned i = 0; released_ids != 0 && i < MaxTouchPoints; i++)
	{
		if (released_ids & BIT(i))
		{
			m_pInterface->ReportHandler (TouchScreenEventFingerUp, i, 0, 0);
			released_ids &= ~BIT(i);
		}
	}

	m_nKnownIDs = active_ids;
}

int CRPiTouchScreen1::regmap_read (u8 reg, int *pval)
{
	return regmap_bulk_read (reg, pval, 1);
}

int CRPiTouchScreen1::regmap_bulk_read (u8 reg, void *buf, size_t buflen)
{
	int ret = m_pI2C->Write (I2CAddress, &reg, 1);
	if (ret != 1)
		return ret;

	return m_pI2C->Read (I2CAddress, buf, buflen) == (int) buflen ? 0 : -1;
}

void CRPiTouchScreen1::UpdateStub (void *pParam)
{
	CRPiTouchScreen1 *pThis = static_cast<CRPiTouchScreen1 *> (pParam);
	assert (pThis);

	pThis->edt_ft5x06_ts_isr ();
}
