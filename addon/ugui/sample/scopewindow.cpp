//
// scopewindow.cpp
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
#include "scopewindow.h"
#include <circle/string.h>
#include <assert.h>

#define WIDTH		600
#define HEIGHT		480
#define MARGIN		5

CScopeWindow::CScopeWindow (UG_S16 sPosX0, UG_S16 sPosY0, CRecorder *pRecorder, CScopeConfig *pConfig)
:	m_pRecorder (pRecorder),
	m_pConfig (pConfig),
	m_nChannelEnable (0),
	m_pTimeLine (0)
{
	assert (m_pRecorder != 0);

	UG_WindowCreate (&m_Window, m_ObjectList, s_ObjectCount, Callback);
	UG_WindowSetStyle (&m_Window, WND_STYLE_2D | WND_STYLE_HIDE_TITLE);
	UG_WindowResize (&m_Window, sPosX0, sPosY0, sPosX0+WIDTH-1, sPosY0+HEIGHT-1);

	UG_TextboxCreate (&m_Window, &m_Textbox1, TXB_ID_0, 5, 5, 149, 25);
	UG_TextboxSetFont (&m_Window, TXB_ID_0, &FONT_10X16);
	UG_TextboxSetText (&m_Window, TXB_ID_0, "TinyScope Pi");
	UG_TextboxSetBackColor (&m_Window, TXB_ID_0, C_LIGHT_GRAY);
	UG_TextboxSetForeColor (&m_Window, TXB_ID_0, C_BLACK);
	UG_TextboxSetAlignment (&m_Window, TXB_ID_0, ALIGN_CENTER);

	UG_WindowShow (&m_Window);

	UG_Update ();

	UpdateChart ();
}

CScopeWindow::~CScopeWindow (void)
{
	UG_WindowHide (&m_Window);
	UG_WindowDelete (&m_Window);

	delete m_pTimeLine;
	m_pTimeLine = 0;
}

void CScopeWindow::UpdateChart (void)
{
	UG_AREA Area;
	UG_WindowGetArea (&m_Window, &Area);
	Area.xs += MARGIN;
	Area.xe -= MARGIN;
	Area.ys += MARGIN+20+10;	// consider title height and margin below
	Area.ye -= MARGIN;

	UG_FillFrame (Area.xs, Area.ys, Area.xe, Area.ye, C_BLACK);

	if (m_pTimeLine == 0)
	{
		return;
	}

	// display headline

	double fZoomFactor = m_pTimeLine->GetZoomFactor ();
	CString Zoom;
	if (fZoomFactor >= 1.0)
	{
		Zoom.Format ("%.0f", fZoomFactor);
	}
	else
	{
		Zoom.Format ("1/%.0f", 1.0 / fZoomFactor);
	}

	double fSampleRate = m_pTimeLine->GetSampleRate ();
	CString SampleRate;
	if (fSampleRate < 1000000.0)
	{
		SampleRate.Format ("%.0fKHz", fSampleRate / 1000.0);
	}
	else
	{
		SampleRate.Format ("%.3fMHz", fSampleRate / 1000000.0);
	}

	CString String;
	String.Format ("SC: %.0fus     OF: %.1fus     TO: %.0fus     ZO: x%s     FQ: %s",
		       m_pTimeLine->GetScaleDivision () * 1000000.0,
		       m_pTimeLine->GetWindowLeft () * 1000000.0,
		       m_pTimeLine->GetRuntime () * 1000000.0,
		       (const char *) Zoom,
		       (const char *) SampleRate);

	UG_SetBackcolor (C_BLACK);
	UG_SetForecolor (C_WHITE);
	UG_FontSelect (&FONT_6X8);
	UG_PutString (Area.xs+5, Area.ys+1, (char *) (const char *) String);

	Area.ys += 12;
	UG_DrawLine (Area.xs, Area.ys, Area.xe, Area.ys, C_WHITE);

	// draw scale

	DrawScale (Area);

	// count active channels

	unsigned nChannelCount = 0;
	for (unsigned nChannel = 1; nChannel <= CHANS; nChannel++)
	{
		if (m_nChannelEnable & CH (nChannel))
		{
			nChannelCount++;
		}
	}

	if (nChannelCount == 0)
	{
		return;
	}

	if (nChannelCount == 1)
	{
		Area.ys += 100;
		Area.ye -= 100;
	}

	// draw channels

	UG_S16 sChannelHeight = (Area.ye-Area.ys+1) / nChannelCount;

	unsigned nChannelsDrawn = 0;
	for (unsigned nChannel = 1; nChannel <= CHANS; nChannel++)
	{
		if (m_nChannelEnable & CH (nChannel))
		{
			UG_AREA ChannelArea;
			ChannelArea.xs = Area.xs;
			ChannelArea.xe = Area.xe;
			ChannelArea.ys = Area.ys + sChannelHeight*nChannelsDrawn;
			ChannelArea.ye = ChannelArea.ys + sChannelHeight-1;

			DrawChannel (nChannel, ChannelArea);

			nChannelsDrawn++;
		}
	}
}

void CScopeWindow::SetChannelEnable (unsigned nMask)
{
	m_nChannelEnable = nMask;

	delete m_pTimeLine;

	assert (m_pConfig != 0);
	assert (m_pRecorder != 0);
	m_pTimeLine = new CTimeLine (m_pConfig->GetMemoryDepth (),
				     (double) m_pRecorder->GetRuntime () / 1000000.0,
				     WIDTH-2*MARGIN);
}

void CScopeWindow::SetChartZoom (int nZoom)
{
	if (m_pTimeLine != 0)
	{
		switch (nZoom)
		{
		case 0:
			m_pTimeLine->SetZoom (TIMELINE_ZOOM_DEFAULT);
			break;

		case -1:
			m_pTimeLine->SetZoom (TIMELINE_ZOOM_OUT);
			break;

		case +1:
			m_pTimeLine->SetZoom (TIMELINE_ZOOM_IN);
			break;

		default:
			assert (0);
			break;
		}
	}
}

void CScopeWindow::SetChartOffsetHome (void)
{
	if (m_pTimeLine != 0)
	{
		m_pTimeLine->SetOffset (0.0);
	}
}

void CScopeWindow::SetChartOffsetEnd (void)
{
	if (m_pTimeLine != 0)
	{
		m_pTimeLine->SetOffset (TIMELINE_MAX_OFFSET);
	}
}

void CScopeWindow::AddChartOffset (int nPixel)
{
	if (m_pTimeLine != 0)
	{
		m_pTimeLine->AddOffset (nPixel);
	}
}

void CScopeWindow::DrawScale (UG_AREA &Area)
{
	assert (m_pTimeLine != 0);

	unsigned nLastMarker = 0;
	for (double fTime = m_pTimeLine->GetWindowLeft ();
	     fTime < m_pTimeLine->GetWindowRight ();
	     fTime += m_pTimeLine->GetPixelDuration () / 2.0)
	{
		unsigned nMarker = (unsigned) (fTime / m_pTimeLine->GetScaleDivision ());

		if (nLastMarker != nMarker)
		{
			unsigned nPixel = m_pTimeLine->GetPixel (fTime);
			assert (nPixel != TIMELINE_INVALID_PIXEL);
			UG_S16 sPosX = Area.xs + nPixel;

			UG_DrawLine (sPosX, Area.ys, sPosX, Area.ye, C_WHITE);

			nLastMarker = nMarker;
		}
	}
}

void CScopeWindow::DrawChannel (unsigned nChannel, UG_AREA &Area)
{
	static const UG_COLOR Colors[] = {C_YELLOW, C_CYAN, C_MAGENTA, C_ORANGE};
	UG_COLOR Color = Colors[(nChannel-1) & 3];

	assert (m_pTimeLine != 0);
	assert (m_pRecorder != 0);

	UG_S16 nLastPosY = 0;
	for (double fTime = m_pTimeLine->GetWindowLeft ();
	     fTime < m_pTimeLine->GetWindowRight ();
	     fTime += m_pTimeLine->GetPixelDuration () / 2.0)
	{
		unsigned nPixel = m_pTimeLine->GetPixel (fTime);
		assert (nPixel != TIMELINE_INVALID_PIXEL);
		UG_S16 sPosX = Area.xs + nPixel;

		unsigned nMeasure = m_pTimeLine->GetMeasure (fTime);
		assert (nMeasure != TIMELINE_INVALID_MEASURE);
		UG_S16 nPosY = m_pRecorder->GetSample (nChannel, nMeasure) == LOW ? Area.ye-5 : Area.ys+5;

		if (nLastPosY == 0)
		{
			nLastPosY = nPosY;
		}

		UG_DrawLine (sPosX, nLastPosY, sPosX, nPosY, Color);

		nLastPosY = nPosY;
	}

	UG_FontSelect (&FONT_8X14);
	UG_PutChar ('0'+nChannel, Area.xs+5, Area.ys+(Area.ye-Area.ys+1-14)/2, C_BLACK, Color);
}

void CScopeWindow::Callback (UG_MESSAGE *pMsg)
{
}
