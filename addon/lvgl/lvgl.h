//
// lvgl.h
//
// C++ wrapper for LVGL with mouse and touch screen support
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2019-2023  R. Stange <rsta2@o2online.de>
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
#ifndef _lvgl_lvgl_h
#define _lvgl_lvgl_h

#include <lvgl/lvgl/lvgl.h>
#include <circle/screen.h>
#include <circle/bcmframebuffer.h>
#include <circle/interrupt.h>
#include <circle/input/mouse.h>
#include <circle/input/touchscreen.h>
#include <circle/dmachannel.h>
#include <circle/types.h>
#include <assert.h>

class CLVGL
{
public:
	CLVGL (CScreenDevice *pScreen, CInterruptSystem *pInterrupt);
	CLVGL (CBcmFrameBuffer *pFrameBuffer, CInterruptSystem *pInterrupt);
	~CLVGL (void);

	boolean Initialize (void);

	void Update (boolean bPlugAndPlayUpdated = FALSE);

private:
	static void DisplayFlush (lv_disp_drv_t *pDriver, const lv_area_t *pArea,
				  lv_color_t *pBuffer);
	static void DisplayFlushComplete (unsigned nChannel, boolean bStatus, void *pParam);

	static void PointerRead (lv_indev_drv_t *pDriver, lv_indev_data_t *pData);
	static void MouseEventHandler (TMouseEvent Event, unsigned nButtons,
				       unsigned nPosX, unsigned nPosY, int nWheelMove);
	static void TouchScreenEventHandler (TTouchScreenEvent Event, unsigned nID,
					     unsigned nPosX, unsigned nPosY);

	static void LogPrint (const char *pMessage);

	static void MouseRemovedHandler (CDevice *pDevice, void *pContext);

	static void PeriodicTickHandler (void);

private:
	lv_color_t *m_pBuffer1;
	lv_color_t *m_pBuffer2;

	CScreenDevice *m_pScreen;
	CBcmFrameBuffer *m_pFrameBuffer;
	CDMAChannel m_DMAChannel;
	unsigned m_nLastUpdate;

	CMouseDevice * volatile m_pMouseDevice;
	CTouchScreenDevice *m_pTouchScreen;
	unsigned m_nLastTouchUpdate;
	lv_indev_data_t m_PointerData;

	static CLVGL *s_pThis;
};

#endif
