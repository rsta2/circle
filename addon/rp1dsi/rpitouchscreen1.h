//
// rpitouchscreen1.h
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
#ifndef _rp1dsi_rpitouchscreen1_h
#define _rp1dsi_rpitouchscreen1_h

#include <circle/display.h>
#include <circle/input/touchscreen.h>
#include <circle/interrupt.h>
#include <circle/i2cmaster.h>
#include <circle/string.h>
#include <circle/types.h>
#include "drm_mipi_dsi.h"
#include "attinyregulator.h"
#include "rp1dsihostcontroller.h"
#include "tc358762dsibridge.h"

class CRPiTouchScreen1 : public mipi_dsi_device		// EDT FT5x06 I2C Touchscreen Driver
{
public:
	CRPiTouchScreen1 (CInterruptSystem *pInterrupt, CI2CMaster *pI2C,
			  unsigned nDepth, unsigned nDisplay,
			  boolean bEnableTouch);
	~CRPiTouchScreen1 (void);

	void SetRotation (unsigned nDegrees)	{ m_nRotation = nDegrees; }
	unsigned GetRotation (void) const	{ return m_nRotation; }

	boolean Initialize (void);
	boolean Start (void *pFrameBuffer, unsigned nPitch);

	unsigned GetWidth (void) const;
	unsigned GetHeight (void) const;

	boolean SetBacklightBrightness (unsigned nBrightness);

	typedef CRP1DSIHostController::TVBlankHandler TVerticalSyncHandler;
	void RegisterVerticalSyncHandler (TVerticalSyncHandler *pHandler, void *pParam);

private:
	int edt_ft5x06_ts_identify (void);
	void edt_ft5x06_ts_isr (void);

	int regmap_read (u8 reg, int *pval);
	int regmap_bulk_read (u8 reg, void *buf, size_t buflen);

	static void UpdateStub (void *pParam);

private:
	static const u8 I2CAddress = 0x38;

	static const unsigned Width = 800;
	static const unsigned Height = 480;

	static const int MaxTouchPoints = 10;

	// for GENERIC_FT
	static const u8 tdata_cmd = 0;
	static const unsigned tdata_len = 3;
	static const unsigned tdata_offset = tdata_len;
	static const unsigned point_len = 6;

	enum edt_ver
	{
		EDT_M06,
		EDT_M09,
		EDT_M12,
		EV_FT,
		GENERIC_FT,
	};

private:
	CInterruptSystem *m_pInterrupt;
	CI2CMaster *m_pI2C;
	boolean m_bEnableTouch;

	unsigned m_nRotation;

	boolean m_bATTinyInitialized;

	CATTinyRegulator m_ATTiny;
	CRP1DSIHostController m_DSI;
	CTC358762DSIBridge m_Bridge;

	CString m_model_name;
	edt_ver m_version;

	int m_init_td_status;
	unsigned m_nKnownIDs;
	unsigned m_nPosX[MaxTouchPoints];
	unsigned m_nPosY[MaxTouchPoints];

	CTouchScreenDevice *m_pInterface;

	static const drm_display_mode s_raspberrypi_7inch_mode;
};

#endif
