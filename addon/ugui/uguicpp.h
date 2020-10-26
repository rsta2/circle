//
// uguicpp.h
//
// C++ wrapper for uGUI with mouse and touch screen support
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2020  R. Stange <rsta2@o2online.de>
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
#ifndef _ugui_uguicpp_h
#define _ugui_uguicpp_h

#ifdef __cplusplus
extern "C" {
#endif
	#include <ugui/ugui.h>
#ifdef __cplusplus
}
#endif

#include <circle/screen.h>
#include <circle/input/mouse.h>
#include <circle/input/touchscreen.h>
#include <circle/types.h>

#if DEPTH != 16
	#error DEPTH must be set to 16 in include/circle/screen.h!
#endif

class CUGUI
{
public:
	CUGUI (CScreenDevice *pScreen);
	~CUGUI (void);

	boolean Initialize (void);

	void Update (boolean bPlugAndPlayUpdated = FALSE);

private:
	static void SetPixel (UG_S16 sPosX, UG_S16 sPosY, UG_COLOR Color);

	void MouseEventHandler (TMouseEvent Event, unsigned nButtons,
				unsigned nPosX, unsigned nPosY, int nWheelMove);
	static void MouseEventStub (TMouseEvent Event, unsigned nButtons,
				    unsigned nPosX, unsigned nPosY, int nWheelMove);

	void TouchScreenEventHandler (TTouchScreenEvent Event,
				      unsigned nID, unsigned nPosX, unsigned nPosY);
	static void TouchScreenEventStub (TTouchScreenEvent Event,
					  unsigned nID, unsigned nPosX, unsigned nPosY);

	static void MouseRemovedHandler (CDevice *pDevice, void *pContext);

private:
	CScreenDevice *m_pScreen;

	UG_GUI m_GUI;

	CMouseDevice * volatile m_pMouseDevice;

	CTouchScreenDevice *m_pTouchScreen;
	unsigned m_nLastUpdate;

	static CUGUI *s_pThis;
};

#endif
