//
// kernel.h
//
#ifndef _kernel_h
#define _kernel_h

#include <circle/startup.h>
#include <circle/actled.h>
#include <circle/koptions.h>
#include <circle/devicenameservice.h>
#include <circle/screen.h>
#include <circle/serial.h>
#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/types.h>
#include <circle/sched/scheduler.h>
#include <circle/sched/synchronizationevent.h>
#include <shell/shell.h>
#include <zxsmi/zxsmi.h>
#include <zxscreen/zxscreen.h>
#include "backgroundtask.h"
#include "screenprocessortask.h"


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
	void AnimationFrame (void);

	// Callbacks
	static void PeriodicTimer100Hz (void);
	static void Reboot (void* pContext);

private:
	// do not change this order
	CActLED			m_ActLED;
	CKernelOptions		m_Options;
	CDeviceNameService	m_DeviceNameService;
	// CScreenDevice		m_Screen;
	CSerialDevice		m_Serial;
	CExceptionHandler	m_ExceptionHandler;
	CInterruptSystem	m_Interrupt;
	CTimer			m_Timer;
	CLogger			m_Logger;

	// TODO: add more members here
	CScheduler		m_Scheduler;
	CSynchronizationEvent	m_Event;
	
	CShell			m_Shell;
	CZxSmi			m_ZxSmi;
	CZxScreen		m_ZxScreen;
	
};

#endif
