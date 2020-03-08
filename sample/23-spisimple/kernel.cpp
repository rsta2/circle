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
#include <circle/debug.h>

#define SPI_MASTER_DEVICE	0		// 0, 4, 5, 6 on Raspberry Pi 4; 0 otherwise

#define SPI_CLOCK_SPEED		500000		// Hz
#define SPI_CPOL		0
#define SPI_CPHA		0

#define SPI_CHIP_SELECT		0		// 0 or 1, or 2 (for SPI1)

#define TEST_DATA_LENGTH	128		// number of data bytes transfered

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
#ifndef USE_SPI_MASTER_AUX
	m_SPIMaster (SPI_CLOCK_SPEED, SPI_CPOL, SPI_CPHA, SPI_MASTER_DEVICE)
#else
	m_SPIMaster (SPI_CLOCK_SPEED)
#endif
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

	m_Logger.Write (FromKernel, LogNotice, "Transfering %u bytes over SPI", TEST_DATA_LENGTH);

	u8 TxData[TEST_DATA_LENGTH];
	u8 RxBuffer[TEST_DATA_LENGTH];
	for (unsigned i = 0; i < TEST_DATA_LENGTH; i++)
	{
		TxData[i] = (u8) i;
		RxBuffer[i] = 0x55;
	}

	if (m_SPIMaster.WriteRead (SPI_CHIP_SELECT, TxData, RxBuffer, TEST_DATA_LENGTH) != TEST_DATA_LENGTH)
	{
		CLogger::Get ()->Write (FromKernel, LogPanic, "SPI write error");
	}

	CLogger::Get ()->Write (FromKernel, LogNotice, "%u bytes transfered", TEST_DATA_LENGTH);

#ifndef NDEBUG
	CLogger::Get ()->Write (FromKernel, LogDebug, "Dumping received data:");

	debug_hexdump (RxBuffer, TEST_DATA_LENGTH, FromKernel);
#endif

	return ShutdownHalt;
}
