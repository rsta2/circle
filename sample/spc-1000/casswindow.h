//
// CassWindow.h
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
#ifndef _CassWindow_h
#define _CassWindow_h

#include <ugui/uguicpp.h>

class CCassWindow
{
public:
	CCassWindow (UG_S16 sPosX0, UG_S16 sPosY0);
	~CCassWindow (void);

	void Update (int);

private:

	static void Callback (UG_MESSAGE *pMsg);

private:

	UG_WINDOW m_Window;

	UG_TEXTBOX m_Textbox1;

	static const unsigned s_ObjectCount = 1;	// must match the number of objects above
	UG_OBJECT m_ObjectList[s_ObjectCount];

	unsigned m_nChannelEnable;
};

#endif
