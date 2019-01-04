//
// bmp180.h
//
// Driver for the BMP180 digital pressure sensor with I2C interface
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2018  R. Stange <rsta2@o2online.de>
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
#ifndef _sensor_bmp180_h
#define _sensor_bmp180_h

#include <circle/i2cmaster.h>
#include <circle/types.h>

class CBMP180
{
public:
	CBMP180 (CI2CMaster *pI2CMaster, unsigned nI2CClockHz = 400000, u8 ucSlaveAddress = 0x77);
	~CBMP180 (void);

	boolean Initialize (void);

	boolean DoMeasurement (void);

	int GetTemperature (void);	// degrees Celsius * 10
	int GetPressure (void);		// hPa * 100

private:
	s32 DoMeasurement (u8 ucCmd, unsigned nDurationMs, unsigned nOversample = 0);
#define BMP180_I2C_ERROR	-1

	boolean WriteRead (u8 ucRegister, void *pBuffer, unsigned nCount);

private:
	CI2CMaster *m_pI2CMaster;
	unsigned    m_nI2CClockHz;
	u8	    m_ucSlaveAddress;

	// calibration data
	s16 m_AC1;
	s16 m_AC2;
	s16 m_AC3;
	u16 m_AC4;
	u16 m_AC5;
	u16 m_AC6;
	s16 m_B1;
	s16 m_B2;
	s16 m_MB;
	s16 m_MC;
	s16 m_MD;

	int m_nTemperature;
	int m_nPressure;
};

#endif
