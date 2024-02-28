//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2023-2024  R. Stange <rsta2@o2online.de>
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
#include <assert.h>

LOGMODULE ("kernel");

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_SerialCDC (&m_Interrupt),
	m_pSerial (nullptr)
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
		bOK = m_SerialCDC.Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	LOGNOTE ("Compile time: " __DATE__ " " __TIME__);

	unsigned nLastTime = 0;
	while (1)
	{
		if (m_SerialCDC.UpdatePlugAndPlay ())
		{
			if (!m_pSerial)
			{
				m_pSerial = static_cast<CUSBSerialDevice *> (
					CDeviceNameService::Get ()->GetDevice ("utty1", FALSE));
				if (m_pSerial)
				{
					m_pSerial->RegisterRemovedHandler (DeviceRemovedHandler,
									   this);

					m_pSerial->SetOptions (SERIAL_OPTION_ONLCR);

					m_Logger.SetNewTarget (m_pSerial);
				}
			}
		}

		// generate a log message every second
		unsigned nNow = m_Timer.GetTime ();
		if (nLastTime != nNow)
		{
			LOGNOTE ("Time is %u", nNow);

			nLastTime = nNow;
		}

		if (m_pSerial)
		{
			char Buffer[100];
			int nResult = m_pSerial->Read (Buffer, sizeof Buffer);
			if (nResult > 0)
			{
				m_Screen.Write (Buffer, nResult);
				m_Serial.Write (Buffer, nResult);
			}
		}
	}

	return ShutdownHalt;
}

void CKernel::DeviceRemovedHandler (CDevice *pDevice, void *pContext)
{
	CKernel *pThis = static_cast<CKernel *> (pContext);
	assert (pThis);

	pThis->m_pSerial = nullptr;

	pThis->m_Logger.SetNewTarget (&pThis->m_Screen);

	LOGDBG ("Device has been detached");
}
