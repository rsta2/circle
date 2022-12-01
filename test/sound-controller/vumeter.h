//
// vumeter.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2022  R. Stange <rsta2@o2online.de>
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
#ifndef _vumeter_h
#define _vumeter_h

#include <circle/screen.h>
#include <circle/types.h>

class CVUMeter		/// Simple non-calibrated VU meter
{
public:
	CVUMeter (CScreenDevice *pScreen,
		  unsigned nPosX, unsigned nPosY,
		  unsigned nWidth, unsigned nHeight);
	~CVUMeter (void);

	void PutInputLevel (float fLevel);

	void Update (void);

private:
	static float exp (float x);

private:
	CScreenDevice *m_pScreen;
	unsigned m_nPosX;
	unsigned m_nPosY;
	unsigned m_nWidth;
	unsigned m_nHeight;

	float m_fFactor;
	float m_fValue;
};

#endif
