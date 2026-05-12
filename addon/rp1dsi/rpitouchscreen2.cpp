// SPDX-License-Identifier: GPL-2.0-only
/*
 * This is based on the Linux driver:
 *	input/touchscreen/goodix.c
 *	by Benjamin Tissoires <benjamin.tissoires@gmail.com>,
 *	   Bastien Nocera <hadess@hadess.net>
 *
 *  Driver for Goodix Touchscreens
 *
 *  Copyright (c) 2014 Red Hat Inc.
 *  Copyright (c) 2015 K. Merker <merker@debian.org>
 *
 *  This code is based on gt9xx.c authored by andrew@goodix.com:
 *
 *  2010 - 2012 Goodix Technology.
 *
 * Ported to Circle by Rene Stange
 */
#include "rpitouchscreen2.h"
#include "linuxcompat.h"
#include <circle/logger.h>
#include <circle/macros.h>
#include <circle/timer.h>
#include <circle/util.h>
#include <assert.h>

#define GOODIX_GT9X_REG_CONFIG_DATA	0x8047
	#define GOODIX_CONFIG_911_LENGTH	186
#define GOODIX_REG_ID			0x8140
	#define GOODIX_ID_MAX_LEN		4
#define GOODIX_READ_COOR_ADDR		0x814E
	#define GOODIX_CONTACT_SIZE		8
	#define GOODIX_MAX_CONTACTS		5

#define GOODIX_BUFFER_STATUS_READY	BIT(7)
#define GOODIX_BUFFER_STATUS_TIMEOUT	20

struct TGoodixContact
{
	u8	id;
	u16	x;
	u16	y;
	u16	width;
	u8	reserved;
}
PACKED;

struct TGoodixReport
{
	u8	contact_num	: 4,
		have_key	: 1,
		reserved	: 2,
		ready		: 1;

	TGoodixContact	contact[GOODIX_MAX_CONTACTS];
}
PACKED;

LOGMODULE ("rp1touch2");

CRPiTouchScreen2::CRPiTouchScreen2 (CInterruptSystem *pInterrupt, CI2CMaster *pI2C,
				    unsigned nDepth, unsigned nDisplay,
				    boolean bEnableTouch,
				    CILI9881Panel::TPanelType PanelType)
:	mipi_dsi_device {&m_DSI, 0, MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_LPM},
	m_pInterrupt (pInterrupt),
	m_pI2C (pI2C),
	m_bEnableTouch (bEnableTouch),
	m_PanelType (PanelType),
	m_bRPiPanel2Initialized (FALSE),
	m_Regulator (pI2C),
	m_DSI (pInterrupt, nDepth, nDisplay, 2),
	m_Panel (this, &m_Regulator),
	m_nKnownIDs (0),
	m_pInterface (nullptr)
{
}

CRPiTouchScreen2::~CRPiTouchScreen2 (void)
{
	delete m_pInterface;
	m_pInterface = nullptr;

	if (m_bRPiPanel2Initialized)
	{
		m_Regulator.SetBacklight (0);
		m_Regulator.GPIOWrite (CRPiPanel2Regulator::CTP_RESET, 0);
	}
}

boolean CRPiTouchScreen2::Initialize (void)
{
	if (!m_Regulator.Initialize ())
	{
		LOGERR ("Cannot init panel regulator");
		return FALSE;
	}
	usleep_range(10, 100);

	m_bRPiPanel2Initialized = TRUE;

	if (m_bEnableTouch)
	{
		if (!m_Regulator.GPIOWrite (CRPiPanel2Regulator::CTP_RESET, 1))
		{
			LOGERR ("Cannot power on touch controller");
			return FALSE;
		}
		usleep_range(5000, 6000);
	}

	if (!m_DSI.Initialize (CILI9881Panel::GetDisplayMode (m_PanelType)))
	{
		LOGERR ("Cannot init DSI host controller");
		return FALSE;
	}

	if (!m_Panel.Initialize (m_PanelType))
	{
		LOGERR ("Cannot init panel");
		return FALSE;
	}

	return TRUE;
}

boolean CRPiTouchScreen2::Start (void *pFrameBuffer, unsigned nPitch)
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

unsigned CRPiTouchScreen2::GetWidth (void) const
{
	return Width;
}

unsigned CRPiTouchScreen2::GetHeight (void) const
{
	return Height;
}

boolean CRPiTouchScreen2::SetBacklightBrightness (unsigned nBrightness)
{
	return m_Regulator.SetBacklight (nBrightness >> 3);
}

void CRPiTouchScreen2::RegisterVerticalSyncHandler (TVerticalSyncHandler *pHandler, void *pParam)
{
	m_DSI.RegisterVBlankHandler (pHandler, pParam);
}

int CRPiTouchScreen2::goodix_i2c_read (u16 reg, u8 *buf, int len)
{
	u16 wbuf = le2be16 (reg);
	int ret;

	ret = m_pI2C->Write (I2CAddress, &wbuf, 2);
	if (ret != 2)
		return -EIO;

	ret = m_pI2C->Read (I2CAddress, buf, len);
	if (ret == len)
		return 0;

	LOGERR ("Error reading %d bytes from 0x%04x: %d",
		len, reg, ret);
	return ret;
}

int CRPiTouchScreen2::goodix_i2c_write (u16 reg, const u8 *buf, int len)
{
	u8 addr_buf[len + 2];
	int ret;

	addr_buf[0] = reg >> 8;
	addr_buf[1] = reg & 0xFF;
	memcpy(&addr_buf[2], buf, len);

	ret = m_pI2C->Write (I2CAddress, addr_buf, len + 2);
	if (ret == len + 2)
		return 0;

	LOGERR ("Error writing %d bytes to 0x%04x: %d",
		len, reg, ret);
	return ret;
}

int CRPiTouchScreen2::goodix_i2c_write_u8 (u16 reg, u8 value)
{
	return goodix_i2c_write(reg, &value, sizeof(value));
}

void CRPiTouchScreen2::goodix_process_events (void)
{
	u8  point_data[2 + GOODIX_CONTACT_SIZE * GOODIX_MAX_CONTACTS];
	int touch_num;

	touch_num = goodix_ts_read_input_report(point_data);
	if (touch_num < 0)
		return;

	/* The pen being down is always reported as a single touch */
	if (touch_num == 1 && (point_data[1] & 0x80)) {
		return;		// ignore pen
	}

	unsigned active_ids = 0;
	unsigned x, y, id;

	for (int i = 0; i < touch_num; i++)
	{
		TGoodixContact *contact =
			(TGoodixContact *) &point_data[i * GOODIX_CONTACT_SIZE + 1];

		x = contact->x & 0xfff;
		y = contact->y & 0xfff;

		id = contact->id;

		active_ids |= BIT(id);

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

int CRPiTouchScreen2::goodix_ts_read_input_report (u8 *data)
{
	unsigned start_ticks;
	int touch_num;
	int error;
	u16 addr = GOODIX_READ_COOR_ADDR;
	/*
	 * We are going to read 1-byte header,
	 * ts->contact_size * max(1, touch_num) bytes of coordinates
	 * and 1-byte footer which contains the touch-key code.
	 */
	const int header_contact_keycode_size = 1 + GOODIX_CONTACT_SIZE + 1;

	/*
	 * The 'buffer status' bit, which indicates that the data is valid, is
	 * not set as soon as the interrupt is raised, but slightly after.
	 * This takes around 10 ms to happen, so we poll for 20 ms.
	 */
	start_ticks = CTimer::GetClockTicks();
	do {
		error = goodix_i2c_read(addr, data,
					header_contact_keycode_size);
		if (error)
			return error;

		if (data[0] & GOODIX_BUFFER_STATUS_READY) {
			touch_num = data[0] & 0x0f;
			if (touch_num > GOODIX_MAX_CONTACTS)
				return -EPROTO;

			if (touch_num > 1) {
				addr += header_contact_keycode_size;
				data += header_contact_keycode_size;
				error = goodix_i2c_read(
						addr, data,
						GOODIX_CONTACT_SIZE *
							(touch_num - 1));
				if (error)
					return error;
			}

			return touch_num;
		}

		usleep_range(1000, 2000); /* Poll every 1 - 2 ms */
	} while (CTimer::GetClockTicks() - start_ticks < GOODIX_BUFFER_STATUS_TIMEOUT * 1000);

	/*
	 * The Goodix panel will send spurious interrupts after a
	 * 'finger up' event, which will always cause a timeout.
	 */
	return -ENOMSG;
}

void CRPiTouchScreen2::UpdateStub (void *pParam)
{
	CRPiTouchScreen2 *pThis = static_cast<CRPiTouchScreen2 *> (pParam);
	assert (pThis);

	pThis->goodix_process_events ();
	pThis->goodix_i2c_write_u8 (GOODIX_READ_COOR_ADDR, 0);
}
