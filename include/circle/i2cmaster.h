//
// i2cmaster.h
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
#ifndef _circle_i2cmaster_h
#define _circle_i2cmaster_h

#include <circle/gpiopin.h>
#include <circle/spinlock.h>
#include <circle/types.h>

// returned by Read/Write as negative value
#define I2C_MASTER_INALID_PARM	1	// Invalid parameter
#define I2C_MASTER_ERROR_NACK	2	// Received a NACK
#define I2C_MASTER_ERROR_CLKT	3	// Received clock stretch timeout
#define I2C_MASTER_DATA_LEFT	4	// Not all data has been sent/received

class CI2CMaster
{
public:
	CI2CMaster (unsigned nDevice,			// 0 on Rev. 1 boards, 1 otherwise
		    boolean bFastMode = FALSE);
	~CI2CMaster (void);

	boolean Initialize (void);

	// modify default clock before specific transfer
	void SetClock (unsigned nClockSpeed);		// in Hz

	// returns number of read bytes or < 0 on failure
	int Read (u8 ucAddress, void *pBuffer, unsigned nCount);

	// returns number of written bytes or < 0 on failure
	int Write (u8 ucAddress, const void *pBuffer, unsigned nCount);

private:
	unsigned m_nDevice;
	unsigned m_nBaseAddress;
	boolean  m_bFastMode;

	CGPIOPin m_SDA;
	CGPIOPin m_SCL;

	unsigned m_nCoreClockRate;

	CSpinLock m_SpinLock;
};

#endif
