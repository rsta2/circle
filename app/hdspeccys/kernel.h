//
// kernel.h
//
#ifndef _kernel_h
#define _kernel_h

#include "../config.h"

#include <circle/startup.h>
#include <circle/actled.h>
#include <circle/koptions.h>
#include <circle/devicenameservice.h>
// #include <circle/screen.h>
#include <circle/serial.h>
#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/gpiomanager.h>
#include <circle/types.h>
#include <circle/sched/scheduler.h>
#include <circle/sched/synchronizationevent.h>
#include <circle/cputhrottle.h>
#include <shell/shell.h>
#include <zxreset/zxreset.h>
#include <zxtape/zxtape.h>
#include <zxsmi/zxsmi.h>
#include "backgroundtask.h"
#include "screenprocessortask.h"
#include "tapetask.h"

#if HD_SPECCYS_FEATURE_NETWORK
#include <circle/usb/usbhcidevice.h>
#include <SDCard/emmc.h>
#include <fatfs/ff.h>
#include <wlan/bcm4343.h>
#include <wlan/hostap/wpa_supplicant/wpasupplicant.h>
#include <circle/net/netsubsystem.h>

#include <zxweb/zxweb.h>
#endif // HD_SPECCYS_FEATURE_NETWORK

#if HD_SPECCYS_FEATURE_OPENGL
#include <vc4/vchiq/vchiqdevice.h>
#else
#include <zxscreen/zxscreen.h>
#endif // HD_SPECCYS_FEATURE_OPENGL


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
	// void AnimationFrame (void);

	// Callbacks
	// static void PeriodicTimer100Hz (void);
	static void Reboot (void* pContext);

private:
	// do not change this order
	CActLED			m_ActLED;
	CKernelOptions		m_Options;
	CDeviceNameService	m_DeviceNameService;
	CCPUThrottle		m_CPUThrottle;
	// CScreenDevice		m_Screen;
	CSerialDevice		m_Serial;
	CExceptionHandler	m_ExceptionHandler;
	CInterruptSystem	m_Interrupt;
	CTimer			m_Timer;
	CLogger			m_Logger;

	// TODO: add more members here
	CGPIOManager	m_GPIOManager;	
	CScheduler		m_Scheduler;
	CSynchronizationEvent	m_Event;

#if HD_SPECCYS_FEATURE_NETWORK
	CUSBHCIDevice		m_USBHCI;
	CEMMCDevice			m_EMMC;
	FATFS				m_FileSystem;
	CBcm4343Device		m_WLAN;
	CNetSubSystem		m_Net;
	CWPASupplicant		m_WPASupplicant;
#endif

	CShell			m_Shell;
	CZxReset		m_ZxReset;
	CZxTape			m_ZxTape;
	CZxSmi			m_ZxSmi;

#if HD_SPECCYS_FEATURE_OPENGL
	CVCHIQDevice		m_VCHIQ;
#else
	CZxScreen		m_ZxScreen;
#endif	
};

#endif // _kernel_h