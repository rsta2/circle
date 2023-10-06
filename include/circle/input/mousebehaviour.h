//
// mousebehaviour.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2023  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_input_mousebehaviour_h
#define _circle_input_mousebehaviour_h

#include <circle/types.h>

enum TMouseEvent
{
	MouseEventMouseMove,
	MouseEventMouseDown,
	MouseEventMouseUp,
	//MouseEventClick,
	//MouseEventDoubleClick,
	MouseEventMouseWheel,
	MouseEventUnknown
};

#define MOUSE_BUTTON_LEFT	(1 << 0)
#define MOUSE_BUTTON_RIGHT	(1 << 1)
#define MOUSE_BUTTON_MIDDLE	(1 << 2)
#define MOUSE_BUTTON_SIDE1	(1 << 3)
#define MOUSE_BUTTON_SIDE2	(1 << 4)

typedef void TMouseEventHandler (TMouseEvent Event, unsigned nButtons, unsigned nPosX, unsigned nPosY, int nWheelMove);

class CMouseBehaviour
{
public:
	CMouseBehaviour (void);
	~CMouseBehaviour (void);

	boolean Setup (unsigned nScreenWidth, unsigned nScreenHeight);	// returns FALSE on failure

	void Release (void);

	void RegisterEventHandler (TMouseEventHandler *pEventHandler);

	boolean SetCursor (unsigned nPosX, unsigned nPosY);		// returns FALSE on failure
	boolean ShowCursor (boolean bShow);				// returns previous state

	void UpdateCursor (void);	// call this frequently from TASK_LEVEL

public:
	void MouseStatusChanged (unsigned nButtons, int nDisplacementX, int nDisplacementY, int nWheelMove);

private:
	static boolean SetCursorState (unsigned nPosX, unsigned nPosY, boolean bVisible);

private:
	unsigned m_nScreenWidth;
	unsigned m_nScreenHeight;

	unsigned m_nPosX;
	unsigned m_nPosY;
	boolean  m_bHasMoved;

	boolean m_bCursorOn;

	unsigned m_nButtons;

	TMouseEventHandler *m_pEventHandler;
};

#endif
