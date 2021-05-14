//
// kernel.h
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
#include <circle/usb/usbgamepad.h>
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
	void ShowDigitalButton (int nPosX, int nPosY, TGamePadButton Button,
				const char *pName = "BUTTON");
	void ShowAxisButton (int nPosX, int nPosY, TGamePadButton Button, TGamePadAxis Axis);
	void ShowAxisControl (int nPosX, int nPosY, TGamePadAxis AxisX,
			      TGamePadAxis AxisY, TGamePadButton Button);
	void ShowGyroscopeData (int nPosX, int nPosY, const int Data[3], const char *pName = 0);

	void PrintAt (int nPosX, int nPosY, const char *pString, boolean bBold = FALSE);

	void MoveTo (int nPosX, int nPosY);
	void LineTo (int nPosX, int nPosY, TScreenColor Color = NORMAL_COLOR);
	void DrawLine (int nPosX1, int nPosY1, int nPosX2, int nPosY2, TScreenColor Color);

	static void GamePadStatusHandler (unsigned nDeviceIndex, const TGamePadState *pState);

	static void GamePadRemovedHandler (CDevice *pDevice, void *pContext);

private:
	// do not change this order
	CActLED			m_ActLED;
	CKernelOptions		m_Options;
	CDeviceNameService	m_DeviceNameService;
	CScreenDevice		m_Screen;
	CSerialDevice		m_Serial;
	CExceptionHandler	m_ExceptionHandler;
	CInterruptSystem	m_Interrupt;
	CTimer			m_Timer;
	CLogger			m_Logger;
	CUSBHCIDevice		m_USBHCI;

	CUSBGamePadDevice * volatile m_pGamePad;
	boolean		   m_bGamePadKnown;
	TGamePadState	   m_GamePadState;

	int m_nPosX;
	int m_nPosY;

	static CKernel *s_pThis;
};

#endif
