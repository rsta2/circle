//
// virtualgpiopin.h
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
#ifndef _virtualgpiopin_h
#define _virtualgpiopin_h

#include <circle/spinlock.h>
#include <circle/types.h>

#define VIRTUAL_GPIO_PINS	2

class CVirtualGPIOPin
{
public:
	// safe mode works with chain boot, but is not as quick
	CVirtualGPIOPin (unsigned nPin, boolean bSafeMode = FALSE); // can be used for output only
	virtual ~CVirtualGPIOPin (void);

	void Write (unsigned nValue);
#define LOW		0
#define HIGH		1

	void Invert (void);

private:
	boolean   m_bSafeMode;

	unsigned  m_nPin;
	unsigned  m_nValue;
	u16	  m_nEnableCount;
	u16	  m_nDisableCount;

	static uintptr s_nGPIOBaseAddress;

	static CSpinLock s_SpinLock;
};

#endif
