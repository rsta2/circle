//
// actled.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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
#ifndef _actled_h
#define _actled_h

#include <circle/gpiopin.h>
#include <circle/types.h>

class CActLED
{
public:
	CActLED (void);
	~CActLED (void);

	void On (void);
	void Off (void);

	static CActLED *Get (void);
	
private:
	CGPIOPin *m_pPin;
	boolean m_bActiveHigh;

	static CActLED *s_pThis;
};

#endif
