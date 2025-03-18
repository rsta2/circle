//
// kernel.h
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
#ifndef _kernel_h
#define _kernel_h

#include <circle/actled.h>
#include <circle/koptions.h>
#include <circle/devicenameservice.h>
#include <circle/screen.h>
#include <circle/serial.h>
#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/usb/usbhcidevice.h>
#include <circle/input/rpitouchscreen.h>
#include <circle/types.h>

#ifdef SPI_DISPLAY
	#include <circle/spimaster.h>
	#include <circle/terminal.h>
	#include <circle/gpiomanager.h>
	#include <circle/input/xpt2046touchscreen.h>
	#include <display/sampleconfig.h>
#endif

enum TShutdownMode
{
	ShutdownNone,
	ShutdownHalt,
	ShutdownReboot
};

class CKernel
{
public:
	CKernel (void);
	~CKernel (void);

	boolean Initialize (void);

	TShutdownMode Run (void);

private:
	static void TouchScreenEventHandler (TTouchScreenEvent Event,
					     unsigned nID, unsigned nPosX, unsigned nPosY);
	
private:
	// do not change this order
	CActLED			m_ActLED;
	CKernelOptions		m_Options;
	CDeviceNameService	m_DeviceNameService;
#ifndef SPI_DISPLAY
	CScreenDevice		m_Screen;
#else
	CSPIMaster		m_SPIMaster;
	DISPLAY_CLASS		m_SPIDisplay;
	CTerminalDevice		m_Screen;
#endif
	CSerialDevice		m_Serial;
	CExceptionHandler	m_ExceptionHandler;
	CInterruptSystem	m_Interrupt;
	CTimer			m_Timer;
	CLogger			m_Logger;
	CUSBHCIDevice		m_USBHCI;

#ifdef SPI_DISPLAY
	CGPIOManager		m_GPIOManager;
	CXPT2046TouchScreen	m_TouchScreen;
#elif RASPPI <= 4
	CRPiTouchScreen		m_RPiTouchScreen;
#endif

	static CKernel *s_pThis;
};

#endif
