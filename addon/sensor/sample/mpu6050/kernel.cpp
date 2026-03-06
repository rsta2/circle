//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2026  R. Stange <rsta2@gmx.net>
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
#include "kernel.h"
#include <circle/machineinfo.h>

#ifdef USE_GRAPHICS
	#include "3dblock.h"
#endif

// may change this on Raspberry Pi 4 to select a specific master device and configuration
#define I2C_MASTER_DEVICE	(CMachineInfo::Get ()->GetDevice (DeviceI2CMaster))
#define I2C_MASTER_CONFIG	0

#define MPU6050_I2C_ADDRESS	0x68
#define MPU6050_I2C_CLOCKHZ	400000

#define ACCELERATION_RANGE	CMPU6050::AccelerationRange4g
#define GYROSCOPE_RANGE		CMPU6050::GyroscopeRange1000		// degrees per second
#define FILTER_BANDWIDTH 	CMPU6050::FilterBandwidth21Hz

#define PI			3.1415926

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_I2CMaster (I2C_MASTER_DEVICE, TRUE, I2C_MASTER_CONFIG),
	m_MPU6050 (&m_I2CMaster, MPU6050_I2C_CLOCKHZ, MPU6050_I2C_ADDRESS)
{
	m_ActLED.Blink (5);	// show we are alive
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;

	if (bOK)
	{
		bOK = m_Screen.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Serial.Initialize (115200);
	}

	if (bOK)
	{
		CDevice *pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
		if (pTarget == 0)
		{
#ifdef USE_GRAPHICS
			pTarget = &m_Serial;
#else
			pTarget = &m_Screen;
#endif
		}

		bOK = m_Logger.Initialize (pTarget);
	}

	if (bOK)
	{
		bOK = m_Interrupt.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Timer.Initialize ();
	}

	if (bOK)
	{
		bOK = m_I2CMaster.Initialize ();
	}

	if (bOK)
	{
		bOK = m_MPU6050.Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	m_MPU6050.WriteAccelerationRange (ACCELERATION_RANGE);
	m_MPU6050.WriteGyroscopeRange (GYROSCOPE_RANGE);
	m_MPU6050.WriteFilterBandwidth (FILTER_BANDWIDTH);

#ifdef USE_GRAPHICS
	C3DBlock Block (1.5, 2.0, 0.15);
#endif

	while (1)
	{
#ifdef USE_GRAPHICS
		m_Screen.ClearScreen (CDisplay::Black);
#endif

		if (m_MPU6050.DoMeasurement ())
		{
			CMPU6050::TResult Accel = m_MPU6050.GetAcceleration ();
			CMPU6050::TResult Gyro = m_MPU6050.GetGyroscopeOutput ();
			float fTemp = m_MPU6050.GetTemperature ();

			m_Logger.Write (FromKernel, LogNotice,
					"Accel %.1f/%.1f/%.1f Gyro %.1f/%.1f/%.1f Temp %.1f",
					Accel.x, Accel.y, Accel.z, Gyro.x, Gyro.y, Gyro.z, fTemp);

#ifdef USE_GRAPHICS
			double x = -Accel.z * PI/2.0;
			double z = (Accel.y-1.0) * PI/2.0;

			Block.SetRotation (x, 0.0, z);

			Block.Draw (&m_Screen, CDisplay::BrightGreen);
#endif
		}
		else
		{
			m_Logger.Write (FromKernel, LogWarning, "Measurement failed");
		}

#ifdef USE_GRAPHICS
		m_Screen.UpdateDisplay ();
#else
		m_Timer.MsDelay (500);
#endif
	}

	return ShutdownHalt;
}
