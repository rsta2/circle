//
// mouse.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2023  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_input_mouse_h
#define _circle_input_mouse_h

#include <circle/device.h>
#include <circle/input/mousebehaviour.h>
#include <circle/numberpool.h>
#include <circle/types.h>

#define MOUSE_DISPLACEMENT_MIN	-127
#define MOUSE_DISPLACEMENT_MAX	127

typedef void TMouseStatusHandlerEx (unsigned nButtons, int nDisplacementX, int nDisplacementY, int nWheelMove, void* pArg);
typedef void TMouseStatusHandler (unsigned nButtons, int nDisplacementX, int nDisplacementY, int nWheelMove);

class CMouseDevice : public CDevice	/// Generic mouse interface device ("mouse1")
{
public:
	/// \brief Construct a mouse device instance
	/// \param nButtons Number of buttons
	/// \param bHasWheel Wheel presence
	CMouseDevice (unsigned nButtons, boolean bHasWheel = FALSE);
	~CMouseDevice (void);

	/// \brief Setup mouse device in cooked mode
	/// \param nScreenWidth  Width of the screen in pixels
	/// \param nScreenHeight Height of the screen in pixels
	/// \return FALSE on failure
	boolean Setup (unsigned nScreenWidth, unsigned nScreenHeight);

	/// \brief Undo Setup()
	/// \note Call this before resizing the screen!
	void Release (void);

	/// \brief Register event handler in cooked mode
	/// \param pEventHandler Pointer to the event handler (see: mousebehaviour.h)
	void RegisterEventHandler (TMouseEventHandler *pEventHandler);

	/// \brief Set mouse cursor to a specific position in cooked mode
	/// \param nPosX X-coordinate of the position in pixels (0 is on the left border)
	/// \param nPosY Y-coordinate of the position in pixels (0 is on the top border)
	/// \return FALSE on failure
	boolean SetCursor (unsigned nPosX, unsigned nPosY);
	/// \brief Switch mouse cursor on or off in cooked mode
	/// \param bShow TRUE shows the mouse cursor
	/// \return Previous state
	boolean ShowCursor (boolean bShow);

	/// \brief Call this frequently from TASK_LEVEL (cooked mode only)
	void UpdateCursor (void);

	/// \brief Register mouse status handler in raw mode
	/// \param pStatusHandler Pointer to the mouse status handler
	/// \param pArg User argument, handed over to the mouse status handler
	void RegisterStatusHandler (TMouseStatusHandlerEx *pStatusHandler, void* pArg);
	void RegisterStatusHandler (TMouseStatusHandler *pStatusHandler);

	/// \return Number of supported mouse buttons
	unsigned GetButtonCount (void) const;

	/// \return Does the mouse support a mouse wheel?
	boolean HasWheel (void) const;

public:
	/// \warning Do not call this from application!
	void ReportHandler (unsigned nButtons, int nDisplacementX, int nDisplacementY, int nWheelMove);

private:
	CMouseBehaviour m_Behaviour;

	TMouseStatusHandlerEx *m_pStatusHandler;
	void* m_pStatusHandlerArg;

	unsigned m_nDeviceNumber;
	static CNumberPool s_DeviceNumberPool;

	unsigned m_nButtons;
	boolean m_bHasWheel;
};

#endif
