//
// casswindow.cpp
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
#include "casswindow.h"
#include <circle/string.h>
#include <assert.h>

#define WIDTH		300
#define HEIGHT		20
#define MARGIN		1

CCassWindow::CCassWindow (UG_S16 sPosX0, UG_S16 sPosY0)
{

	UG_WindowCreate (&m_Window, m_ObjectList, s_ObjectCount, Callback);
	UG_WindowSetStyle (&m_Window, WND_STYLE_2D | WND_STYLE_HIDE_TITLE);
	UG_WindowResize (&m_Window, sPosX0, sPosY0, sPosX0+WIDTH-1, sPosY0+HEIGHT-1);

	UG_TextboxCreate (&m_Window, &m_Textbox1, TXB_ID_0, 5, 5, 149, 25);
	UG_TextboxSetFont (&m_Window, TXB_ID_0, &FONT_10X16);
	UG_TextboxSetText (&m_Window, TXB_ID_0, "Cassette Tape");
	UG_TextboxSetBackColor (&m_Window, TXB_ID_0, C_GRAY);
	UG_TextboxSetForeColor (&m_Window, TXB_ID_0, C_BLACK);
	UG_TextboxSetAlignment (&m_Window, TXB_ID_0, ALIGN_CENTER);

	UG_WindowShow (&m_Window);

	UG_Update ();

}

CCassWindow::~CCassWindow (void)
{
	UG_WindowHide (&m_Window);
	UG_WindowDelete (&m_Window);
}

void CCassWindow::Update (int progress)
{
	UG_AREA Area;
	UG_WindowGetArea (&m_Window, &Area);
	Area.xs += MARGIN;
	Area.xe -= MARGIN;
	Area.ys += MARGIN+1+1;	// consider title height and margin below
	Area.ye -= MARGIN;
	int width = (Area.xe - Area.ye) * progress / 100;

	UG_FillFrame (Area.xs, Area.ys, Area.xe, Area.ye, C_BLACK);
	UG_FillFrame (Area.xs, Area.ys, width, Area.ye - 1, C_WHITE);

}

void CCassWindow::Callback (UG_MESSAGE *pMsg)
{
}
