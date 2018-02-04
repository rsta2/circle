//
// mpu6050.cpp
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
#include <sensor/mpu6050.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <assert.h>

// Registers
#define ACCEL_XOUT_H		59
#define ACCEL_XOUT_L		60
#define ACCEL_YOUT_H		61
#define ACCEL_YOUT_L		62
#define ACCEL_ZOUT_H		63
#define ACCEL_ZOUT_L		64
#define TEMP_OUT_H		65
#define TEMP_OUT_L		66
#define GYRO_XOUT_H		67
#define GYRO_XOUT_L		68
#define GYRO_YOUT_H		69
#define GYRO_YOUT_L		70
#define GYRO_ZOUT_H		71
#define GYRO_ZOUT_L		72

#define PWR_MGMT_1		107

static const char FromMPU6050[] = "mpu6050";

CMPU6050::CMPU6050 (CI2CMaster *pI2CMaster, unsigned nI2CClockHz, u8 ucSlaveAddress)
:	m_pI2CMaster (pI2CMaster),
	m_nI2CClockHz (nI2CClockHz),
	m_ucSlaveAddress (ucSlaveAddress)
{
	m_pRegs = new TMPU6050Regs;
	assert (m_pRegs != 0);
}

CMPU6050::~CMPU6050 (void)
{
	delete m_pRegs;
	m_pRegs = 0;

	m_pI2CMaster = 0;
}

boolean CMPU6050::Initialize (void)
{
	assert (m_pI2CMaster != 0);
	m_pI2CMaster->SetClock (m_nI2CClockHz);

	// wake up MPU-6050, set CLKSEL to gyroscope PLL
	u8 Data[] = {PWR_MGMT_1, 0x01};
	int nResult = m_pI2CMaster->Write (m_ucSlaveAddress, Data, sizeof Data);
	if (nResult != sizeof Data)
	{
		CLogger::Get ()->Write (FromMPU6050, LogError, "Device is not present (addr 0x%02X)",
					(unsigned) m_ucSlaveAddress);

		return FALSE;
	}

	CTimer::Get ()->MsDelay (50);		// first measurement may be wrong otherwise

	return TRUE;
}

boolean CMPU6050::DoMeasurement (void)
{
	assert (m_pI2CMaster != 0);
	m_pI2CMaster->SetClock (m_nI2CClockHz);

	u8 Data[] = {ACCEL_XOUT_H};
	int nResult = m_pI2CMaster->Write (m_ucSlaveAddress, Data, sizeof Data);
	if (nResult != sizeof Data)
	{
		CLogger::Get ()->Write (FromMPU6050, LogError, "I2C write failed (err %d)", nResult);

		return FALSE;
	}

	assert (m_pRegs != 0);
	assert (sizeof *m_pRegs == GYRO_ZOUT_L-ACCEL_XOUT_H+1);
	nResult = m_pI2CMaster->Read (m_ucSlaveAddress, m_pRegs, sizeof *m_pRegs);
	if (nResult != sizeof *m_pRegs)
	{
		CLogger::Get ()->Write (FromMPU6050, LogError, "I2C read failed (err %d)", nResult);

		return FALSE;
	}

	return TRUE;
}

s16 CMPU6050::GetAccelerationX (void) const
{
	assert (m_pRegs != 0);
	return (s16) m_pRegs->AccelXOutH << 8 | m_pRegs->AccelXOutL;
}

s16 CMPU6050::GetAccelerationY (void) const
{
	assert (m_pRegs != 0);
	return (s16) m_pRegs->AccelYOutH << 8 | m_pRegs->AccelYOutL;
}

s16 CMPU6050::GetAccelerationZ (void) const
{
	assert (m_pRegs != 0);
	return (s16) m_pRegs->AccelZOutH << 8 | m_pRegs->AccelZOutL;
}

s16 CMPU6050::GetGyroscopeOutputX (void) const
{
	assert (m_pRegs != 0);
	return (s16) m_pRegs->GyroXOutH << 8 | m_pRegs->GyroXOutL;
}

s16 CMPU6050::GetGyroscopeOutputY (void) const
{
	assert (m_pRegs != 0);
	return (s16) m_pRegs->GyroYOutH << 8 | m_pRegs->GyroYOutL;
}

s16 CMPU6050::GetGyroscopeOutputZ (void) const
{
	assert (m_pRegs != 0);
	return (s16) m_pRegs->GyroZOutH << 8 | m_pRegs->GyroZOutL;
}
