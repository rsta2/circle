//
// kernel.h
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
#include <circle/i2cmaster.h>
#include <circle/gpiopin.h>
#include <circle/types.h>
#include <gpio/mcp23017.h>

enum TShutdownMode
{
	ShutdownNone,
	ShutdownHalt,
	ShutdownReboot
};

// MCP23017 configuration
#define MCP23017_I2C_ADDRESS	0x21
#define INTA_PIN		16		// GPIO16 (header pin 36)
#define INTB_PIN		26		// GPIO26 (header pin 37)

class CKernel
{
public:
	CKernel (void);
	~CKernel (void);

	boolean Initialize (void);

	TShutdownMode Run (void);

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
	CI2CMaster		m_I2CMaster;

	CMCP23017		m_MCP23017;
	CGPIOPin		m_IntAPin;
	CGPIOPin		m_IntBPin;
};

#endif
