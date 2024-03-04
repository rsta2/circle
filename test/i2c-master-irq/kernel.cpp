//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2024  R. Stange <rsta2@o2online.de>
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
#include <assert.h>

// may change this on Raspberry Pi 4 to select a specific master device and configuration
#define I2C_MASTER_DEVICE	(CMachineInfo::Get ()->GetDevice (DeviceI2CMaster))
#define I2C_MASTER_CONFIG	0

#define I2C_ADDRESS		0x77		// I2C address of BMP180
#define I2C_CLOCKHZ		400000

#define WRITE_BYTES		{0xD0}		// chip ID register of BMP180
#define READ_COUNT		1		// BMP180 must return 0x55

LOGMODULE ("kernel");

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_I2CMaster (&m_Interrupt, I2C_MASTER_DEVICE, TRUE, I2C_MASTER_CONFIG)
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

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	LOGNOTE ("Compile time: " __DATE__ " " __TIME__);

	m_I2CMaster.SetClock (I2C_CLOCKHZ);

	m_I2CMaster.SetCompletionRoutine (I2CCompletionRoutine, this);

	static const u8 WriteBuffer[] = WRITE_BYTES;
	u8 ReadBuffer[READ_COUNT];

	m_bTransferDone = FALSE;
	int nStatus = m_I2CMaster.StartWriteRead (I2C_ADDRESS,
						  WriteBuffer, sizeof WriteBuffer,
						  ReadBuffer, sizeof ReadBuffer);
	if (nStatus)
	{
		LOGPANIC ("Cannot start I2C operation (%d)", nStatus);
	}

	while (!m_bTransferDone)
	{
		LOGDBG ("Waiting ...");
	}

	nStatus = m_I2CMaster.GetStatus ();
	if (nStatus)
	{
		LOGPANIC ("I2C operation failed (%d)", nStatus);
	}

	LOGNOTE ("Dumping received data:");

	for (size_t i = 0; i < sizeof ReadBuffer; i++)
	{
		LOGNOTE ("%lu: 0x%02X", i, ReadBuffer[i]);
	}

	return ShutdownHalt;
}

void CKernel::I2CCompletionRoutine (int nStatus, void *pParam)
{
	CKernel *pThis = static_cast<CKernel *> (pParam);
	assert (pThis);

	LOGNOTE ("I2C operation completed (%d)", nStatus);

	pThis->m_bTransferDone = TRUE;
}
