//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
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
#include <circle/string.h>

#define CHANNEL			0		// 0..7 (0..3 for MCP3004)
#define VREF			3.3f		// Reference voltage (Volt)

#define SPI_MASTER_DEVICE	0		// 0, 4, 5, 6 on Raspberry Pi 4; 0 otherwise
#define SPI_CHIP_SELECT		0		// 0 or 1, or 2 (for SPI1)
#define SPI_CLOCK_SPEED		1000000		// Hz

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
#ifndef USE_SPI_MASTER_AUX
	m_SPIMaster (SPI_CLOCK_SPEED, 0, 0, SPI_MASTER_DEVICE),
#else
	m_SPIMaster (SPI_CLOCK_SPEED),
#endif
	m_MCP300X (&m_SPIMaster, VREF, SPI_CHIP_SELECT, SPI_CLOCK_SPEED)
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
		bOK = m_SPIMaster.Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	while (1)
	{
		float fResult = m_MCP300X.DoSingleEndedConversion (CHANNEL);
		int nResultRaw = m_MCP300X.DoSingleEndedConversionRaw (CHANNEL);

		if (   fResult < CMCP300X::ResultSPIError
		    && nResultRaw >= 0)
		{
			CString Msg;
			Msg.Format ("%.2fV (raw %d)\n", fResult, nResultRaw);

			m_Screen.Write (Msg, Msg.GetLength ());
		}
		else
		{
			m_Logger.Write (FromKernel, LogWarning, "SPI error");
		}

		m_Timer.MsDelay (1000);
	}

	return ShutdownHalt;
}
