//
// rpitouchscreen2.h
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
#ifndef _rp1dsi_rpitouchscreen2_h
#define _rp1dsi_rpitouchscreen2_h

#include <circle/display.h>
#include <circle/input/touchscreen.h>
#include <circle/interrupt.h>
#include <circle/i2cmaster.h>
#include <circle/types.h>
#include "drm_mipi_dsi.h"
#include "rpipanel2regulator.h"
#include "rp1dsihostcontroller.h"
#include "ili9881panel.h"

class CRPiTouchScreen2 : public mipi_dsi_device	// Driver for Raspberry Pi Touchscreen v2
{
public:
	CRPiTouchScreen2 (CInterruptSystem *pInterrupt, CI2CMaster *pI2C,
			 unsigned nDepth, unsigned nDisplay,
			 boolean bEnableTouch,
			 CILI9881Panel::TPanelType PanelType);
	~CRPiTouchScreen2 (void);

	boolean Initialize (void);
	boolean Start (void *pFrameBuffer, unsigned nPitch);

	unsigned GetWidth (void) const;
	unsigned GetHeight (void) const;

	boolean SetBacklightBrightness (unsigned nBrightness);

	typedef CRP1DSIHostController::TVBlankHandler TVerticalSyncHandler;
	void RegisterVerticalSyncHandler (TVerticalSyncHandler *pHandler, void *pParam = nullptr);

private:
	int goodix_i2c_read (u16 reg, u8 *buf, int len);
	int goodix_i2c_write (u16 reg, const u8 *buf, int len);
	int goodix_i2c_write_u8 (u16 reg, u8 value);

	void goodix_process_events (void);
	int goodix_ts_read_input_report (u8 *data);

	static void UpdateStub (void *pParam);

private:
	static const u8 I2CAddress = 0x5D;

	static const unsigned Width = 720;
	static const unsigned Height = 1280;

	static const int MaxTouchPoints = 5;

private:
	CInterruptSystem *m_pInterrupt;
	CI2CMaster *m_pI2C;
	boolean m_bEnableTouch;
	CILI9881Panel::TPanelType m_PanelType;

	boolean m_bRPiPanel2Initialized;

	CRPiPanel2Regulator m_Regulator;
	CRP1DSIHostController m_DSI;
	CILI9881Panel m_Panel;

	unsigned m_nKnownIDs;
	unsigned m_nPosX[MaxTouchPoints];
	unsigned m_nPosY[MaxTouchPoints];

	CTouchScreenDevice *m_pInterface;
};

#endif
