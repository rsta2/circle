//
// kernel.cpp
//
// MCP23017 GPIO expander test
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2025  T. Nelissen
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
#include <circle/util.h>
#include <circle/machineinfo.h>

static const char FromKernel[] = "kernel";

#define I2C_MASTER_DEVICE	(CMachineInfo::Get ()->GetDevice (DeviceI2CMaster))

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_I2CMaster (I2C_MASTER_DEVICE),
	m_MCP23017 (&m_I2CMaster, MCP23017_I2C_ADDRESS),
	m_IntAPin (INTA_PIN, GPIOModeInputPullUp),
	m_IntBPin (INTB_PIN, GPIOModeInputPullUp)
{
	m_ActLED.Blink (5);
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
		bOK = m_MCP23017.Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);
	m_Logger.Write (FromKernel, LogNotice, "MCP23017 test started (I2C addr 0x%02X)",
			MCP23017_I2C_ADDRESS);
	m_Logger.Write (FromKernel, LogNotice, "INTA=GPIO%u  INTB=GPIO%u",
			INTA_PIN, INTB_PIN);

	// Test 1: Read initial port state
	m_Logger.Write (FromKernel, LogNotice, "--- Test 1: Read initial port state ---");
	u8 uchPortA = m_MCP23017.ReadPortA ();
	u8 uchPortB = m_MCP23017.ReadPortB ();
	m_Logger.Write (FromKernel, LogNotice,
			"Port A: 0x%02X (%u%u%u%u%u%u%u%u)",
			(unsigned) uchPortA,
			(uchPortA >> 7) & 1, (uchPortA >> 6) & 1,
			(uchPortA >> 5) & 1, (uchPortA >> 4) & 1,
			(uchPortA >> 3) & 1, (uchPortA >> 2) & 1,
			(uchPortA >> 1) & 1, (uchPortA >> 0) & 1);
	m_Logger.Write (FromKernel, LogNotice,
			"Port B: 0x%02X (%u%u%u%u%u%u%u%u)",
			(unsigned) uchPortB,
			(uchPortB >> 7) & 1, (uchPortB >> 6) & 1,
			(uchPortB >> 5) & 1, (uchPortB >> 4) & 1,
			(uchPortB >> 3) & 1, (uchPortB >> 2) & 1,
			(uchPortB >> 1) & 1, (uchPortB >> 0) & 1);
	CTimer::SimpleMsDelay (1000);

	// Test 2: Configure port A as output, write pattern
	m_Logger.Write (FromKernel, LogNotice, "--- Test 2: Port A output test ---");
	m_MCP23017.SetDirectionA (0x00);	// All outputs

	m_Logger.Write (FromKernel, LogNotice, "Writing 0xAA to port A...");
	m_MCP23017.WritePortA (0xAA);
	CTimer::SimpleMsDelay (1000);

	m_Logger.Write (FromKernel, LogNotice, "Writing 0x55 to port A...");
	m_MCP23017.WritePortA (0x55);
	CTimer::SimpleMsDelay (1000);

	m_Logger.Write (FromKernel, LogNotice, "Writing 0xFF to port A...");
	m_MCP23017.WritePortA (0xFF);
	CTimer::SimpleMsDelay (1000);

	m_Logger.Write (FromKernel, LogNotice, "Writing 0x00 to port A...");
	m_MCP23017.WritePortA (0x00);
	CTimer::SimpleMsDelay (1000);

	// Reset port A back to inputs
	m_MCP23017.SetDirectionA (0xFF);
	m_MCP23017.SetPullUpA (0xFF);

	// Test 3: Polling loop — read ports and monitor interrupts
	m_Logger.Write (FromKernel, LogNotice,
			"--- Test 3: Polling (press buttons / toggle inputs) ---");
	m_Logger.Write (FromKernel, LogNotice,
			"Monitoring ports for 30 seconds...");

	u8 uchPrevA = m_MCP23017.ReadPortA ();
	u8 uchPrevB = m_MCP23017.ReadPortB ();

	unsigned nStartTime = m_Timer.GetTime ();
	unsigned nLastPrint = 0;

	while (m_Timer.GetTime () - nStartTime < 30)
	{
		// Check interrupt pins
		unsigned nIntA = m_IntAPin.Read ();
		unsigned nIntB = m_IntBPin.Read ();

		boolean bChanged = FALSE;

		if (nIntA == LOW)
		{
			u8 uchFlag = m_MCP23017.ReadInterruptFlagA ();
			u8 uchCap  = m_MCP23017.ReadInterruptCaptureA ();
			m_Logger.Write (FromKernel, LogNotice,
					"INTA! Flag=0x%02X Capture=0x%02X",
					(unsigned) uchFlag, (unsigned) uchCap);
			bChanged = TRUE;
		}

		if (nIntB == LOW)
		{
			u8 uchFlag = m_MCP23017.ReadInterruptFlagB ();
			u8 uchCap  = m_MCP23017.ReadInterruptCaptureB ();
			m_Logger.Write (FromKernel, LogNotice,
					"INTB! Flag=0x%02X Capture=0x%02X",
					(unsigned) uchFlag, (unsigned) uchCap);
			bChanged = TRUE;
		}

		// Also poll port values directly
		uchPortA = m_MCP23017.ReadPortA ();
		uchPortB = m_MCP23017.ReadPortB ();

		if (uchPortA != uchPrevA || uchPortB != uchPrevB)
		{
			m_Logger.Write (FromKernel, LogNotice,
					"Port change: A=0x%02X B=0x%02X",
					(unsigned) uchPortA, (unsigned) uchPortB);
			uchPrevA = uchPortA;
			uchPrevB = uchPortB;
			bChanged = TRUE;
		}

		// Print periodic status every 5 seconds
		unsigned nElapsed = m_Timer.GetTime () - nStartTime;
		if (nElapsed >= nLastPrint + 5)
		{
			nLastPrint = nElapsed;
			m_Logger.Write (FromKernel, LogNotice,
					"[%us] A=0x%02X B=0x%02X INTA=%s INTB=%s",
					nElapsed,
					(unsigned) uchPortA, (unsigned) uchPortB,
					nIntA == LOW ? "ACTIVE" : "idle",
					nIntB == LOW ? "ACTIVE" : "idle");
		}

		if (!bChanged)
		{
			CTimer::SimpleMsDelay (10);
		}
	}

	m_Logger.Write (FromKernel, LogNotice, "All MCP23017 tests completed!");

	return ShutdownHalt;
}
