//
// scopewindow.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016  R. Stange <rsta2@o2online.de>
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
#ifndef _scopewindow_h
#define _scopewindow_h

#include <ugui/uguicpp.h>
#include "recorder.h"
#include "scopeconfig.h"
#include "timeline.h"

class CScopeWindow
{
public:
	CScopeWindow (UG_S16 sPosX0, UG_S16 sPosY0, CRecorder *pRecorder, CScopeConfig *pConfig);
	~CScopeWindow (void);

	void UpdateChart (void);

	void SetChannelEnable (unsigned nMask);

	void SetChartZoom (int nZoom);			// -1 zoom out, +1 zoom in, 0 set default

	// set or move left border of chart display
	void SetChartOffsetHome (void);
	void SetChartOffsetEnd (void);
	void AddChartOffset (int nPixel);

private:
	void DrawScale (UG_AREA &Area);
	void DrawChannel (unsigned nChannel, UG_AREA &Area);

	static void Callback (UG_MESSAGE *pMsg);

private:
	CRecorder *m_pRecorder;
	CScopeConfig *m_pConfig;

	UG_WINDOW m_Window;

	UG_TEXTBOX m_Textbox1;

	static const unsigned s_ObjectCount = 1;	// must match the number of objects above
	UG_OBJECT m_ObjectList[s_ObjectCount];

	unsigned m_nChannelEnable;

	CTimeLine *m_pTimeLine;
};

#endif
