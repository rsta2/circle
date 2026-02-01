//
// kernel.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2025  R. Stange <rsta2@gmx.net>
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

#if RASPPI < 5
	#error This sample does only run on the Raspberry Pi 5.
#endif

#define TEST_DUMP_MBR	1		// Dump the Master Boot Block
#define TEST_RAW_READ	2		// Read raw blocks
#define TEST_CIRCLE_FS	3		// Use native FAT FS of Circle
#define TEST_FATFS	4		// Use Chan's FatFs

#define TESTSEL		TEST_DUMP_MBR

#define TEST_RO		1		// Read-only test (0 or 1)

#include <circle/actled.h>
#include <circle/koptions.h>
#include <circle/devicenameservice.h>
#include <circle/screen.h>
#include <circle/serial.h>
#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/sched/scheduler.h>
#include <circle/device.h>
#include <nvme/nvme.h>
#include <circle/types.h>

#if TESTSEL == TEST_FATFS
	#include <fatfs/ff.h>
#elif TESTSEL == TEST_CIRCLE_FS
	#include <circle/fs/fat/fatfs.h>
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
#ifdef NO_BUSY_WAIT
	CScheduler		m_Scheduler;
#endif
	CDevice			*m_pTarget;

	CNVMeDevice		m_NVMe;

#if TESTSEL == TEST_FATFS
	FATFS			m_FileSystem;
#elif TESTSEL == TEST_CIRCLE_FS
	CFATFileSystem		m_FileSystem;
#endif
};

#endif
