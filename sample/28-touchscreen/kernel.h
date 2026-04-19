//
// kernel.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2026  R. Stange <rsta2@gmx.net>
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
#ifndef DSI_DISPLAY
#include <circle/screen.h>
#else
#include <circle/terminal.h>
#endif
#include <circle/serial.h>
#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/usb/usbhcidevice.h>
#ifndef DSI_DISPLAY
#include <circle/input/rpitouchscreen.h>
#else
#include <rp1dsi/rpitouchscreen.h>
#endif
#include <circle/types.h>

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
	CInterruptSystem	m_Interrupt;
#ifndef DSI_DISPLAY
	CScreenDevice		m_Screen;
#else
	CRPiTouchScreen		m_RPiTouchScreen;
	CTerminalDevice		m_Screen;
#endif
	CSerialDevice		m_Serial;
	CExceptionHandler	m_ExceptionHandler;
	CTimer			m_Timer;
	CLogger			m_Logger;
	CUSBHCIDevice		m_USBHCI;

#if RASPPI <= 4
	CRPiTouchScreen		m_RPiTouchScreen;
#endif

	static CKernel *s_pThis;
};

#endif
