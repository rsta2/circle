//
// serial.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2016  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_serial_h
#define _circle_serial_h

#include <circle/device.h>
#include <circle/gpiopin.h>
#include <circle/spinlock.h>
#include <circle/sysconfig.h>
#include <circle/types.h>

class CSerialDevice : public CDevice
{
public:
	CSerialDevice (void);
	~CSerialDevice (void);

	boolean Initialize (unsigned nBaudrate = 115200);

	int Write (const void *pBuffer, unsigned nCount);

#ifndef USE_RPI_STUB_AT
private:
	void Write (u8 nChar);

private:
#if RASPPI >= 2
	CGPIOPin m_GPIO32;
	CGPIOPin m_GPIO33;
#endif
	CGPIOPin m_TxDPin;
	CGPIOPin m_RxDPin;

	CSpinLock m_SpinLock;
#endif
};

#endif
