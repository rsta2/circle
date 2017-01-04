//
// controlwindow.cpp
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
#include "controlwindow.h"
#include <assert.h>

#define TXB_VERTICAL	TXB_ID_0
#define TXB_VERT_CHANS	TXB_ID_1
#define TXB_HORIZONTAL	TXB_ID_2
#define TXB_TRIGGER	TXB_ID_3
#define TXB_TRIG_CHANS	TXB_ID_4
#define TXB_ACQUIRE	TXB_ID_5
#define TXB_RATE	TXB_ID_6

#define BTN_LEFT_QUICK	BTN_ID_0
#define BTN_LEFT	BTN_ID_1
#define BTN_RIGHT	BTN_ID_2
#define BTN_RIGHT_QUICK	BTN_ID_3
#define BTN_HOME	BTN_ID_4
#define BTN_END		BTN_ID_5
#define BTN_ZOOM_IN	BTN_ID_6
#define BTN_ZOOM_OUT	BTN_ID_7
#define BTN_RATE_LEFT	BTN_ID_8
#define BTN_RATE_RIGHT	BTN_ID_9
#define BTN_RUN		BTN_ID_10

#define CHB_VERT_CH1	CHB_ID_0
#define CHB_VERT_CH2	CHB_ID_1
#define CHB_VERT_CH3	CHB_ID_2
#define CHB_VERT_CH4	CHB_ID_3
#define CHB_TRIG_EN_CH1	CHB_ID_4
#define CHB_TRIG_EN_CH2	CHB_ID_5
#define CHB_TRIG_EN_CH3	CHB_ID_6
#define CHB_TRIG_EN_CH4	CHB_ID_7
#define CHB_TRIG_LV_CH1	CHB_ID_8
#define CHB_TRIG_LV_CH2	CHB_ID_9
#define CHB_TRIG_LV_CH3	CHB_ID_10
#define CHB_TRIG_LV_CH4	CHB_ID_11

CControlWindow *CControlWindow::s_pThis = 0;

CControlWindow::CControlWindow (UG_S16 sPosX0, UG_S16 sPosY0,
				CScopeWindow *pScopeWindow, CRecorder *pRecorder, CScopeConfig *pConfig)
:	m_pScopeWindow (pScopeWindow),
	m_pRecorder (pRecorder),
	m_pConfig (pConfig),
	m_nChannelEnable (CH1 | CH2),
	m_nParamIndex (m_pConfig->GetParamSetCount ()-1)
{
	assert (s_pThis == 0);
	s_pThis = this;

	// create window
	UG_WindowCreate (&m_Window, m_ObjectList, s_ObjectCount, CallbackStub);
	UG_WindowSetStyle (&m_Window, WND_STYLE_2D | WND_STYLE_HIDE_TITLE);
	UG_WindowResize (&m_Window, sPosX0, sPosY0, sPosX0+199, sPosY0+479);

	// create controls
	UG_TextboxCreate (&m_Window, &m_Textbox1, TXB_VERTICAL, 5, 5, 194, 25);
	UG_TextboxCreate (&m_Window, &m_Textbox2, TXB_VERT_CHANS, 5, 35, 194, 55);
	UG_CheckboxCreate (&m_Window, &m_Checkbox1[0], CHB_VERT_CH1, 18, 60, 60, 80);
	UG_CheckboxCreate (&m_Window, &m_Checkbox1[1], CHB_VERT_CH2, 65, 60, 107, 80);
	UG_CheckboxCreate (&m_Window, &m_Checkbox1[2], CHB_VERT_CH3, 114, 60, 156, 80);
	UG_CheckboxCreate (&m_Window, &m_Checkbox1[3], CHB_VERT_CH4, 162, 60, 199, 80);
	UG_TextboxCreate (&m_Window, &m_Textbox3, TXB_HORIZONTAL, 5, 120, 194, 140);
	UG_ButtonCreate (&m_Window, &m_Button1, BTN_LEFT_QUICK, 10, 150, 50, 175);
	UG_ButtonCreate (&m_Window, &m_Button2, BTN_LEFT, 58, 150, 98, 175);
	UG_ButtonCreate (&m_Window, &m_Button3, BTN_RIGHT, 106, 150, 146, 175);
	UG_ButtonCreate (&m_Window, &m_Button4, BTN_RIGHT_QUICK, 154, 150, 194, 175);
	UG_ButtonCreate (&m_Window, &m_Button5, BTN_HOME, 10, 180, 50, 205);
	UG_ButtonCreate (&m_Window, &m_Button6, BTN_END, 58, 180, 98, 205);
	UG_ButtonCreate (&m_Window, &m_Button7, BTN_ZOOM_IN, 106, 180, 146, 205);
	UG_ButtonCreate (&m_Window, &m_Button8, BTN_ZOOM_OUT, 154, 180, 194, 205);
	UG_TextboxCreate (&m_Window, &m_Textbox4, TXB_TRIGGER, 5, 245, 194, 265);
	UG_TextboxCreate (&m_Window, &m_Textbox5, TXB_TRIG_CHANS, 5, 275, 194, 295);
	UG_CheckboxCreate (&m_Window, &m_Checkbox2[0], CHB_TRIG_EN_CH1, 7, 300, 37, 320);
	UG_CheckboxCreate (&m_Window, &m_Checkbox2[1], CHB_TRIG_EN_CH2, 38, 300, 69, 320);
	UG_CheckboxCreate (&m_Window, &m_Checkbox2[2], CHB_TRIG_EN_CH3, 70, 300, 101, 320);
	UG_CheckboxCreate (&m_Window, &m_Checkbox2[3], CHB_TRIG_EN_CH4, 103, 300, 199, 320);
	UG_CheckboxCreate (&m_Window, &m_Checkbox3[0], CHB_TRIG_LV_CH1, 7, 325, 37, 345);
	UG_CheckboxCreate (&m_Window, &m_Checkbox3[1], CHB_TRIG_LV_CH2, 38, 325, 69, 345);
	UG_CheckboxCreate (&m_Window, &m_Checkbox3[2], CHB_TRIG_LV_CH3, 70, 325, 101, 345);
	UG_CheckboxCreate (&m_Window, &m_Checkbox3[3], CHB_TRIG_LV_CH4, 103, 325, 199, 345);
	UG_TextboxCreate (&m_Window, &m_Textbox6, TXB_ACQUIRE, 5, 385, 194, 405);
	UG_ButtonCreate (&m_Window, &m_Button9, BTN_RATE_LEFT, 10, 415, 50, 440);
	UG_TextboxCreate (&m_Window, &m_Textbox7, TXB_RATE, 55, 415, 150, 440);
	UG_ButtonCreate (&m_Window, &m_Button10, BTN_RATE_RIGHT, 155, 415, 194, 440);
	UG_ButtonCreate (&m_Window, &m_Button11, BTN_RUN, 10, 450, 194, 474);

	// "Vertical" section
	UG_TextboxSetFont (&m_Window, TXB_VERTICAL, &FONT_8X14);
	UG_TextboxSetText (&m_Window, TXB_VERTICAL, "VERTICAL");
	UG_TextboxSetBackColor (&m_Window, TXB_VERTICAL, C_MEDIUM_AQUA_MARINE);
	UG_TextboxSetForeColor (&m_Window, TXB_VERTICAL, C_WHITE);
	UG_TextboxSetAlignment (&m_Window, TXB_VERTICAL, ALIGN_CENTER);

	UG_TextboxSetFont (&m_Window, TXB_VERT_CHANS, &FONT_8X14);
	UG_TextboxSetText (&m_Window, TXB_VERT_CHANS, "CH1   CH2   CH3   CH4");
	UG_TextboxSetForeColor (&m_Window, TXB_VERT_CHANS, C_BLACK);
	UG_TextboxSetAlignment (&m_Window, TXB_VERT_CHANS, ALIGN_CENTER);

	UG_CheckboxSetFont (&m_Window, CHB_VERT_CH1, &FONT_8X14);
	UG_CheckboxSetText (&m_Window, CHB_VERT_CH1, "");
	UG_CheckboxSetFont (&m_Window, CHB_VERT_CH2, &FONT_8X14);
	UG_CheckboxSetText (&m_Window, CHB_VERT_CH2, "");
	UG_CheckboxSetFont (&m_Window, CHB_VERT_CH3, &FONT_8X14);
	UG_CheckboxSetText (&m_Window, CHB_VERT_CH3, "");
	UG_CheckboxSetFont (&m_Window, CHB_VERT_CH4, &FONT_8X14);
	UG_CheckboxSetText (&m_Window, CHB_VERT_CH4, "");

	for (unsigned nChannel = 1; nChannel <= CHANS; nChannel++)
	{
		if (m_nChannelEnable & CH (nChannel))
		{
			UG_CheckboxSetCheched (&m_Window, CHB_VERT_CH1+nChannel-1, 1);
		}
	}

	// "Horizontal" section
	UG_TextboxSetFont (&m_Window, TXB_HORIZONTAL, &FONT_8X14);
	UG_TextboxSetText (&m_Window, TXB_HORIZONTAL, "HORIZONTAL");
	UG_TextboxSetBackColor (&m_Window, TXB_HORIZONTAL, C_MEDIUM_AQUA_MARINE);
	UG_TextboxSetForeColor (&m_Window, TXB_HORIZONTAL, C_WHITE);
	UG_TextboxSetAlignment (&m_Window, TXB_HORIZONTAL, ALIGN_CENTER);

	UG_ButtonSetFont (&m_Window, BTN_LEFT_QUICK, &FONT_8X14);
	UG_ButtonSetText (&m_Window, BTN_LEFT_QUICK, "<<");
	UG_ButtonSetFont (&m_Window, BTN_LEFT, &FONT_8X14);
	UG_ButtonSetText (&m_Window, BTN_LEFT, "<");
	UG_ButtonSetFont (&m_Window, BTN_RIGHT, &FONT_8X14);
	UG_ButtonSetText (&m_Window, BTN_RIGHT, ">");
	UG_ButtonSetFont (&m_Window, BTN_RIGHT_QUICK, &FONT_8X14);
	UG_ButtonSetText (&m_Window, BTN_RIGHT_QUICK, ">>");
	UG_ButtonSetFont (&m_Window, BTN_HOME, &FONT_8X14);
	UG_ButtonSetText (&m_Window, BTN_HOME, "|<");
	UG_ButtonSetFont (&m_Window, BTN_END, &FONT_8X14);
	UG_ButtonSetText (&m_Window, BTN_END, ">|");
	UG_ButtonSetFont (&m_Window, BTN_ZOOM_IN, &FONT_8X14);
	UG_ButtonSetText (&m_Window, BTN_ZOOM_IN, "+");
	UG_ButtonSetFont (&m_Window, BTN_ZOOM_OUT, &FONT_8X14);
	UG_ButtonSetText (&m_Window, BTN_ZOOM_OUT, "-");

	// "Trigger" section
	UG_TextboxSetFont (&m_Window, TXB_TRIGGER, &FONT_8X14);
	UG_TextboxSetText (&m_Window, TXB_TRIGGER, "TRIGGER");
	UG_TextboxSetBackColor (&m_Window, TXB_TRIGGER, C_MEDIUM_AQUA_MARINE);
	UG_TextboxSetForeColor (&m_Window, TXB_TRIGGER, C_WHITE);
	UG_TextboxSetAlignment (&m_Window, TXB_TRIGGER, ALIGN_CENTER);

	UG_TextboxSetFont (&m_Window, TXB_TRIG_CHANS, &FONT_8X14);
	UG_TextboxSetText (&m_Window, TXB_TRIG_CHANS, "CH1 CH2 CH3 CH4");
	UG_TextboxSetForeColor (&m_Window, TXB_TRIG_CHANS, C_BLACK);
	UG_TextboxSetAlignment (&m_Window, TXB_TRIG_CHANS, ALIGN_CENTER_LEFT);

	UG_CheckboxSetFont (&m_Window, CHB_TRIG_EN_CH1, &FONT_8X14);
	UG_CheckboxSetText (&m_Window, CHB_TRIG_EN_CH1, "");
	UG_CheckboxSetFont (&m_Window, CHB_TRIG_EN_CH2, &FONT_8X14);
	UG_CheckboxSetText (&m_Window, CHB_TRIG_EN_CH2, "");
	UG_CheckboxSetFont (&m_Window, CHB_TRIG_EN_CH3, &FONT_8X14);
	UG_CheckboxSetText (&m_Window, CHB_TRIG_EN_CH3, "");
	UG_CheckboxSetFont (&m_Window, CHB_TRIG_EN_CH4, &FONT_8X14);
	UG_CheckboxSetText (&m_Window, CHB_TRIG_EN_CH4, "ENABLE");

	UG_CheckboxSetFont (&m_Window, CHB_TRIG_LV_CH1, &FONT_8X14);
	UG_CheckboxSetText (&m_Window, CHB_TRIG_LV_CH1, "");
	UG_CheckboxSetFont (&m_Window, CHB_TRIG_LV_CH2, &FONT_8X14);
	UG_CheckboxSetText (&m_Window, CHB_TRIG_LV_CH2, "");
	UG_CheckboxSetFont (&m_Window, CHB_TRIG_LV_CH3, &FONT_8X14);
	UG_CheckboxSetText (&m_Window, CHB_TRIG_LV_CH3, "");
	UG_CheckboxSetFont (&m_Window, CHB_TRIG_LV_CH4, &FONT_8X14);
	UG_CheckboxSetText (&m_Window, CHB_TRIG_LV_CH4, "LEVEL");

	// "Acquire" section
	UG_TextboxSetFont (&m_Window, TXB_ACQUIRE, &FONT_8X14);
	UG_TextboxSetText (&m_Window, TXB_ACQUIRE, "ACQUIRE");
	UG_TextboxSetBackColor (&m_Window, TXB_ACQUIRE, C_MEDIUM_AQUA_MARINE);
	UG_TextboxSetForeColor (&m_Window, TXB_ACQUIRE, C_WHITE);
	UG_TextboxSetAlignment (&m_Window, TXB_ACQUIRE, ALIGN_CENTER);

	UG_TextboxSetFont (&m_Window, TXB_RATE, &FONT_8X14);
	UpdateRate (0);
	UG_TextboxSetBackColor (&m_Window, TXB_RATE, C_LIGHT_GRAY);
	UG_TextboxSetForeColor (&m_Window, TXB_RATE, C_BLACK);
	UG_TextboxSetAlignment (&m_Window, TXB_RATE, ALIGN_CENTER);

	UG_ButtonSetFont (&m_Window, BTN_RATE_LEFT, &FONT_8X14);
	UG_ButtonSetText (&m_Window, BTN_RATE_LEFT, "<");
	UG_ButtonSetFont (&m_Window, BTN_RATE_RIGHT, &FONT_8X14);
	UG_ButtonSetText (&m_Window, BTN_RATE_RIGHT, ">");
	UG_ButtonSetFont (&m_Window, BTN_RUN, &FONT_8X14);
	UG_ButtonSetText (&m_Window, BTN_RUN, "RUN");

	UG_WindowShow (&m_Window);

	UG_Update ();
}

CControlWindow::~CControlWindow (void)
{
	UG_WindowHide (&m_Window);
	UG_WindowDelete (&m_Window);

	s_pThis = 0;
}

void CControlWindow::Callback (UG_MESSAGE *pMsg)
{
	assert (pMsg != 0);
	if (   pMsg->type  == MSG_TYPE_OBJECT
	    && pMsg->id    == OBJ_TYPE_BUTTON
	    && pMsg->event == OBJ_EVENT_PRESSED)
	{
		assert (m_pScopeWindow != 0);

		switch (pMsg->sub_id)
		{
		case BTN_LEFT:
			m_pScopeWindow->AddChartOffset (-10);
			break;

		case BTN_RIGHT:
			m_pScopeWindow->AddChartOffset (10);
			break;

		case BTN_LEFT_QUICK:
			m_pScopeWindow->AddChartOffset (-100);
			break;

		case BTN_RIGHT_QUICK:
			m_pScopeWindow->AddChartOffset (100);
			break;

		case BTN_HOME:
			m_pScopeWindow->SetChartOffsetHome ();
			break;

		case BTN_END:
			m_pScopeWindow->SetChartOffsetEnd ();
			break;

		case BTN_ZOOM_IN:
			m_pScopeWindow->SetChartZoom (+1);
			break;

		case BTN_ZOOM_OUT:
			m_pScopeWindow->SetChartZoom (-1);
			break;

		case BTN_RATE_LEFT:
			UpdateRate (-1);
			return;

		case BTN_RATE_RIGHT:
			UpdateRate (1);
			return;

		case BTN_RUN:
			Run ();
			break;

		default:
			return;
		}

		m_pScopeWindow->UpdateChart ();
	}
}

void CControlWindow::CallbackStub (UG_MESSAGE *pMsg)
{
	assert (s_pThis != 0);
	s_pThis->Callback (pMsg);
}

void CControlWindow::Run (void)
{
	u32 nEnable = 0;
	u32 nLevel = 0;
	for (unsigned nChannel = 1; nChannel <= CHANS; nChannel++)
	{
		if (UG_CheckboxGetChecked (&m_Window, CHB_VERT_CH1+nChannel-1))
		{
			m_nChannelEnable |= CH (nChannel);
		}
		else
		{
			m_nChannelEnable &= ~CH (nChannel);
		}

		if (UG_CheckboxGetChecked (&m_Window, CHB_TRIG_EN_CH1+nChannel-1))
		{
			nEnable |= CH (nChannel);
		}

		if (UG_CheckboxGetChecked (&m_Window, CHB_TRIG_LV_CH1+nChannel-1))
		{
			nLevel |= CH (nChannel);
		}
	}

	if (m_nChannelEnable != 0)
	{
		assert (m_pRecorder != 0);
		m_pRecorder->SetTrigger (nEnable, nLevel);

		assert (m_pConfig != 0);
		m_pConfig->SelectParamSet (m_nParamIndex);
		m_pRecorder->SetMemoryDepth (m_pConfig->GetMemoryDepth ());
		m_pRecorder->SetDelayCount (m_pConfig->GetDelayCount ());

		if (m_pRecorder->Run ())
		{
			m_pScopeWindow->SetChannelEnable (m_nChannelEnable);
			m_pScopeWindow->SetChartZoom (0);
			m_pScopeWindow->SetChartOffsetHome ();
		}
	}
}

void CControlWindow::UpdateRate (int nShift)
{
	assert (m_pConfig != 0);

	switch (nShift)
	{
	case 0:
		break;

	case -1:
		if (m_nParamIndex > 0)
		{
			m_nParamIndex--;
		}
		break;

	case 1:
		if (m_nParamIndex+1 < m_pConfig->GetParamSetCount ())
		{
			m_nParamIndex++;
		}
		break;

	default:
		assert (0);
		break;
	}

	unsigned nActualRateKHz = m_pConfig->GetActualRate (m_nParamIndex);
	if (nActualRateKHz < 950)
	{
		m_Rate.Format ("%u KHz", (nActualRateKHz+50) / 100 * 100);
	}
	else
	{
		m_Rate.Format ("%u MHz", (nActualRateKHz+500) / 1000);
	}

	UG_TextboxSetText (&m_Window, TXB_RATE, (char *) (const char *) m_Rate);
}
