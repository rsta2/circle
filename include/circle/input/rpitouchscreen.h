//
// rpitouchscreen.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2021  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_input_rpitouchscreen_h
#define _circle_input_rpitouchscreen_h

#include <circle/input/touchscreen.h>
#include <circle/types.h>

#define RPITOUCH_SCREEN_MAX_POINTS	10

struct TFT5406Buffer;

class CRPiTouchScreen		/// Driver for the official Raspberry Pi touch screen
{
public:
	CRPiTouchScreen (void);
	~CRPiTouchScreen (void);

	boolean Initialize (void);

private:
	void Update (void);	// call this about 60 times per second

	static void UpdateStub (void *pParam);

private:
	TFT5406Buffer *m_pFT5406Buffer;

	unsigned m_nKnownIDs;

	unsigned m_nPosX[RPITOUCH_SCREEN_MAX_POINTS];
	unsigned m_nPosY[RPITOUCH_SCREEN_MAX_POINTS];

	CTouchScreenDevice *m_pDevice;
};

#endif
