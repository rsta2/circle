//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
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

// may change this on Raspberry Pi 4 to select a specific master device and configuration
#define I2C_MASTER_DEVICE	(CMachineInfo::Get ()->GetDevice (DeviceI2CMaster))
#define I2C_MASTER_CONFIG	0

#define BMP180_I2C_ADDRESS	0x77
#define BMP180_I2C_CLOCKHZ	400000

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_I2CMaster (I2C_MASTER_DEVICE, TRUE, I2C_MASTER_CONFIG),
	m_BMP180 (&m_I2CMaster, BMP180_I2C_CLOCKHZ, BMP180_I2C_ADDRESS)
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
			pTarget = &m_Screen;
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
		bOK = m_BMP180.Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	while (1)
	{
		if (m_BMP180.DoMeasurement ())
		{
			m_Logger.Write (FromKernel, LogNotice,
					"Temperature: %.1f \xB0""C, Pressure: %.2f hPa",
					(double) m_BMP180.GetTemperature () / 10.0,
					(double) m_BMP180.GetPressure () / 100.0);
		}
		else
		{
			m_Logger.Write (FromKernel, LogWarning, "Measurement failed");
		}

		m_Timer.MsDelay (500);
	}

	return ShutdownHalt;
}
