//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2024  R. Stange <rsta2@o2online.de>
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
#include <assert.h>

#define XPT2046_CHIP_SELECT	1	// 0 or 1
#define XPT2046_IRQ_PIN		25	// SoC number (not header position!)

static const char FromKernel[] = "kernel";

CKernel *CKernel::s_pThis = 0;

CKernel::CKernel (void)
:
#ifndef SPI_DISPLAY
	m_Screen (0, 0),	// auto-detect size
#else
	m_SPIMaster (SPI_CLOCK_SPEED, SPI_CPOL, SPI_CPHA, SPI_MASTER_DEVICE),
	m_SPIDisplay (&m_SPIMaster, DISPLAY_PARAMETERS),
	m_Screen (&m_SPIDisplay),
#endif
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_USBHCI (&m_Interrupt, &m_Timer)
#ifdef SPI_DISPLAY
	, m_GPIOManager (&m_Interrupt),
	m_TouchScreen (&m_SPIMaster, XPT2046_CHIP_SELECT, &m_GPIOManager, XPT2046_IRQ_PIN)
#endif
{
	s_pThis = this;

	m_ActLED.Blink (5);	// show we are alive
}

CKernel::~CKernel (void)
{
	s_pThis = 0;
}

boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;

#ifdef SPI_DISPLAY
	if (bOK)
	{
		bOK = m_SPIMaster.Initialize ();
	}

	if (bOK)
	{
#ifdef DISPLAY_ROTATION
		m_SPIDisplay.SetRotation (DISPLAY_ROTATION);
#endif

		bOK = m_SPIDisplay.Initialize ();
	}
#endif

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
		bOK = m_USBHCI.Initialize ();
	}

#ifdef SPI_DISPLAY
	if (bOK)
	{
		bOK = m_GPIOManager.Initialize ();
	}

	if (bOK)
	{
		m_TouchScreen.SetRotation (DISPLAY_ROTATION);

		bOK = m_TouchScreen.Initialize ();
	}
#elif RASPPI <= 4
	if (bOK)
	{
		bOK = m_RPiTouchScreen.Initialize ();
	}
#endif

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	CTouchScreenDevice *pTouchScreen =
		(CTouchScreenDevice *) m_DeviceNameService.GetDevice ("touch1", FALSE);
	if (pTouchScreen == 0)
	{
		m_Logger.Write (FromKernel, LogPanic, "Touchscreen not found");
	}

#ifndef SPI_DISPLAY
	pTouchScreen->Setup (m_Screen.GetFrameBuffer ());
#else
	pTouchScreen->Setup (&m_SPIDisplay);
#endif
	const unsigned *pCalibration = m_Options.GetTouchScreen ();
	if (pCalibration != 0)
	{
		if (!pTouchScreen->SetCalibration (pCalibration))
		{
			m_Logger.Write (FromKernel, LogPanic, "Invalid calibration info");
		}
	}

	pTouchScreen->RegisterEventHandler (TouchScreenEventHandler);

	m_Logger.Write (FromKernel, LogNotice, "Just use your touchscreen!");

	for (unsigned nCount = 0; 1; nCount++)
	{
		pTouchScreen->Update ();

#ifndef SPI_DISPLAY
		m_Screen.Rotor (0, nCount);
#endif
		m_Timer.MsDelay (1000/60);
	}

	return ShutdownHalt;
}

void CKernel::TouchScreenEventHandler (TTouchScreenEvent Event,
				       unsigned nID, unsigned nPosX, unsigned nPosY)
{
	assert (s_pThis != 0);

	CString Message;

	switch (Event)
	{
	case TouchScreenEventFingerDown:
		Message.Format ("Finger #%u down at %u / %u", nID+1, nPosX, nPosY);
		break;

	case TouchScreenEventFingerUp:
		Message.Format ("Finger #%u up", nID+1);
		break;

	case TouchScreenEventFingerMove:
		Message.Format ("Finger #%u moved to %u / %u", nID+1, nPosX, nPosY);
		break;

	default:
		assert (0);
		break;
	}

	s_pThis->m_Logger.Write (FromKernel, LogNotice, Message);
}
