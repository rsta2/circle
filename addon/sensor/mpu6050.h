//
// mpu6050.h
//
// Driver for MPU-6050 (and MPU-6500) with I2C interface
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017  R. Stange <rsta2@o2online.de>
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
#ifndef _sensor_mpu6050_h
#define _sensor_mpu6050_h

#include <circle/i2cmaster.h>
#include <circle/types.h>
#include <circle/macros.h>

struct TMPU6050Regs
{
	u8	AccelXOutH;
	u8	AccelXOutL;
	u8	AccelYOutH;
	u8	AccelYOutL;
	u8	AccelZOutH;
	u8	AccelZOutL;
	u8	TempOutH;
	u8	TempOutL;
	u8	GyroXOutH;
	u8	GyroXOutL;
	u8	GyroYOutH;
	u8	GyroYOutL;
	u8	GyroZOutH;
	u8	GyroZOutL;
}
PACKED;

class CMPU6050
{
public:
	CMPU6050 (CI2CMaster *pI2CMaster, unsigned nI2CClockHz = 400000, u8 ucSlaveAddress = 0x68);
	~CMPU6050 (void);

	boolean Initialize (void);

	boolean DoMeasurement (void);

	s16 GetAccelerationX (void) const;
	s16 GetAccelerationY (void) const;
	s16 GetAccelerationZ (void) const;

	s16 GetGyroscopeOutputX (void) const;
	s16 GetGyroscopeOutputY (void) const;
	s16 GetGyroscopeOutputZ (void) const;

private:
	CI2CMaster *m_pI2CMaster;
	unsigned    m_nI2CClockHz;
	u8	    m_ucSlaveAddress;

	TMPU6050Regs *m_pRegs;
};

#endif
