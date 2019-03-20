//
// actled.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2019  R. Stange <rsta2@o2online.de>
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
#include <circle/virtualgpiopin.h>
#include <circle/types.h>

class CActLED
{
public:
	// safe mode works with LED connected to GPIO expander and chain boot,
	// but is not as quick
	CActLED (boolean bSafeMode = FALSE);
	~CActLED (void);

	void On (void);
	void Off (void);

	void Blink (unsigned nCount, unsigned nTimeOnMs = 200, unsigned nTimeOffMs = 500);

	static CActLED *Get (void);
	
private:
	CGPIOPin *m_pPin;
	CVirtualGPIOPin *m_pVirtualPin;
	boolean m_bActiveHigh;

	static CActLED *s_pThis;
};

#endif
