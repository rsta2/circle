//
// touchscreen.h
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
#ifndef _circle_input_touchscreen_h
#define _circle_input_touchscreen_h

#include <circle/device.h>
#include <circle/types.h>

enum TTouchScreenEvent
{
	TouchScreenEventFingerDown,
	TouchScreenEventFingerUp,
	TouchScreenEventFingerMove,
	TouchScreenEventUnknown
};

typedef void TTouchScreenEventHandler (TTouchScreenEvent Event,
				       unsigned nID, unsigned nPosX, unsigned nPosY);
#define TOUCH_SCREEN_MAX_POINTS		10
#define TOUCH_SCREEN_MAX_ID		(TOUCH_SCREEN_MAX_POINTS-1)

struct TFT5406Buffer;

class CTouchScreenDevice : public CDevice
{
public:
	CTouchScreenDevice (void);
	~CTouchScreenDevice (void);

	boolean Initialize (void);

	void Update (void);		// call this about 60 times per second

	void RegisterEventHandler (TTouchScreenEventHandler *pEventHandler);

private:
	TFT5406Buffer *m_pFT5406Buffer;

	TTouchScreenEventHandler *m_pEventHandler;

	unsigned m_nKnownIDs;

	unsigned m_nPosX[TOUCH_SCREEN_MAX_POINTS];
	unsigned m_nPosY[TOUCH_SCREEN_MAX_POINTS];
};

#endif
