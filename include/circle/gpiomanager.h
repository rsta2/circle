//
// gpiomanager.h
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
#ifndef _gpiomanager_h
#define _gpiomanager_h

#include <circle/interrupt.h>
#include <circle/gpiopin.h>
#include <circle/types.h>

class CGPIOManager
{
public:
	CGPIOManager (CInterruptSystem *pInterrupt);
	~CGPIOManager (void);

	boolean Initialize (void);

private:
	void ConnectInterrupt (CGPIOPin *pPin);
	void DisconnectInterrupt (CGPIOPin *pPin);
	friend class CGPIOPin;

	void InterruptHandler (void);
	static void InterruptStub (void *pParam);

private:
	CInterruptSystem *m_pInterrupt;
	boolean m_bIRQConnected;

	CGPIOPin *m_apPin[GPIO_PINS];
};

#endif
