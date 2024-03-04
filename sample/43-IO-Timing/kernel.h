//
// kernel.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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

//#include <circle/multicore.h>
#include <circle/actled.h>
#include <circle/types.h>
#include <circle/screen.h>
#include <circle/timer.h>
#include <circle/startup.h>
#include <circle/koptions.h>
#include <circle/devicenameservice.h>
#include <circle/serial.h>
#include <circle/logger.h>
#include <circle/cputhrottle.h>
#include <stdio.h>

enum TShutdownMode
{
	ShutdownNone,
	ShutdownHalt,
	ShutdownReboot
};

typedef void (*fn_ptr)(void);

class CKernel
{
public:
	CKernel (void);
	~CKernel (void);

	boolean Initialize (void);
	void exec_test(int loopcount, const char *name, fn_ptr f);
	
	TShutdownMode Run (void);

private:
	CKernelOptions mOptions;
	CDeviceNameService m_DeviceNameService;
	CScreenDevice mScreen;
	CSerialDevice mSerial;
	CInterruptSystem mInterrupt;
	CTimer mTimer;
	CLogger mLogger;
};

#endif
