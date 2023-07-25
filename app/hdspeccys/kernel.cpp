//
// kernel.cpp
//
#include "kernel.h"


LOGMODULE ("kernel");

#define HOSTNAME						"hd-speccys"
#define DRIVE							"SD:"
#define FIRMWARE_PATH					DRIVE "/firmware/"		// firmware files must be provided here
#define WPA_SUPPLICANT_CONFIG_FILE		DRIVE "/wpa_supplicant.conf"

static volatile boolean bReboot = TRUE;
static CKernel *pKernel = 0;

#if HD_SPECCYS_FEATURE_OPENGL
extern "C" int _main (void);
#endif // HD_SPECCYS_FEATURE_OPENGL

#if HD_SPECCYS_FEATURE_NETWORK
#if HD_SPECCYS_DHCP
#else
// TODO get from options string or read from file
static const u8 IPAddress[]      = {192, 168, 0, 250};
static const u8 NetMask[]        = {255, 255, 255, 0};
static const u8 DefaultGateway[] = {192, 168, 0, 1};
static const u8 DNSServer[]      = {192, 168, 0, 1};
#endif // HD_SPECCYS_DHCP
#endif // HD_SPECCYS_FEATURE_NETWORK



CKernel::CKernel (void)
:	m_CPUThrottle(CPUSpeedLow),
	// m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	// m_Serial (&m_Interrupt),
	m_Timer (&m_Interrupt),	
	m_Logger (m_Options.GetLogLevel(), &m_Timer),	
	// TODO: add more member initializers here
	m_GPIOManager(&m_Interrupt),
#if HD_SPECCYS_FEATURE_NETWORK
	m_USBHCI (&m_Interrupt, &m_Timer),
	m_EMMC (&m_Interrupt, &m_Timer, &m_ActLED),
	m_WLAN (FIRMWARE_PATH),
#if HD_SPECCYS_DHCP
	m_Net (0, 0, 0, 0, HOSTNAME, NetDeviceTypeWLAN),
#else
	m_Net (IPAddress, NetMask, DefaultGateway, DNSServer, NetDeviceTypeWLAN),
#endif // HD_SPECCYS_DHCP
	m_WPASupplicant (WPA_SUPPLICANT_CONFIG_FILE),
#endif // HD_SPECCYS_FEATURE_NETWORK
	m_Shell (&m_Serial),
	m_ZxTape(&m_GPIOManager, &m_Interrupt),
	m_ZxSmi(&m_GPIOManager, &m_Interrupt),
#if HD_SPECCYS_FEATURE_OPENGL
	m_VCHIQ (CMemorySystem::Get (), &m_Interrupt)
#else
	// m_ZxScreen(m_Options.GetWidth (), m_Options.GetHeight (), FALSE, 0, &m_Interrupt)
	// 320x256 (32 + 256 + 32 x 32 + 192 + 32) - border is 1/3rd screen (this is about right for original speccy)
	m_ZxScreen(320, 256, 0, &m_Interrupt)
	// m_ZxScreen(320 + 128, 256, 0, &m_Interrupt)
	// m_ZxScreen(320 + 144, 256, 0, &m_Interrupt)
	// m_ZxScreen(320*2, 256*2, 0, &m_Interrupt)
	//  m_ZxScreen (m_Options.GetWidth (), m_Options.GetHeight (), 0, &m_Interrupt)
#endif // !(HD_SPECCYS_FEATURE_OPENGL)
{
	bReboot = TRUE;

	pKernel = this;


	// m_ActLED.Blink (5);	// show we are alive	
}

CKernel::~CKernel (void)
{	
}

boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;

	// if (bOK)
	// {
	// 	bOK = m_Screen.Initialize ();
	// }

	if (bOK)
	{
		bOK = m_Serial.Initialize (HD_SPECCYS_SERIAL_BAUD_RATE);
	}

	if (bOK)
	{
		CDevice *pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
		if (pTarget == 0)
		{
			pTarget = &m_Serial;
		}

		bOK = m_Logger.Initialize (pTarget);
	}

	if (bOK)
	{
		bOK = m_Shell.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Interrupt.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Timer.Initialize ();
	}

	// TODO: call Initialize () of added members here (if required)
	if (bOK)
	{
		bOK = m_GPIOManager.Initialize ();
	}

#if HD_SPECCYS_FEATURE_OPENGL
	if (bOK)
	{
		bOK = m_VCHIQ.Initialize ();
	}
#else
	if (bOK)
	{
		bOK = m_ZxScreen.Initialize ();
		m_ZxScreen.SetActLED(&m_ActLED);
	}
#endif // HD_SPECCYS_FEATURE_OPENGL

#if HD_SPECCYS_FEATURE_NETWORK
	if (bOK)
	{
		bOK = m_USBHCI.Initialize ();
	}

	if (bOK)
	{
		bOK = m_EMMC.Initialize ();
	}

	if (bOK)
	{
		if (f_mount (&m_FileSystem, DRIVE, 1) != FR_OK)
		{
			LOGERR("Cannot mount drive: %s", DRIVE);

			bOK = FALSE;
		}
	}

	if (bOK)
	{
		bOK = m_WLAN.Initialize ();
	}

#ifdef USE_OPEN_NET // TODO - get from options string
	if (bOK)
	{
		bOK = m_WLAN.JoinOpenNet (USE_OPEN_NET);
	}
#endif // USE_OPEN_NET

	if (bOK)
	{
		bOK = m_Net.Initialize (FALSE);
	}

#ifndef USE_OPEN_NET
	if (bOK)
	{
		bOK = m_WPASupplicant.Initialize ();
	}
#endif // USE_OPEN_NET
#endif // HD_SPECCYS_FEATURE_NETWORK
	
	if (bOK)
	{
		bOK = m_ZxTape.Initialize ();
	}

	if (bOK)
	{
		bOK = m_ZxSmi.Initialize ();
		m_ZxSmi.SetActLED(&m_ActLED);
	}


	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	LOGNOTE("Compile time: " __DATE__ " " __TIME__);

	// DO NOT LEAVE RUNNING WITH THIS!
	// m_CPUThrottle.SetSpeed(CPUSpeedMaximum);

	new CBackgroundTask (&m_Shell, &m_ActLED, &m_Event);
	// new CTapeTask (&m_ZxTape);

#if HD_SPECCYS_FEATURE_OPENGL
	// OpenGL ES test
	_main ();
#else
	new CScreenProcessorTask (&m_ZxScreen, &m_ZxSmi, &m_ActLED);
#endif	
	

#if HD_SPECCYS_FEATURE_NETWORK
	// DO NOT LEAVE RUNNING WITH THIS!
	// m_CPUThrottle.SetSpeed(CPUSpeedMaximum);

	while (!m_Net.IsRunning ())
	{
		m_Scheduler.MsSleep (100);
	}

	CString IPString;
	m_Net.GetConfig ()->GetIPAddress ()->Format (&IPString);
	LOGNOTE("Open \"http://%s/\" in your web browser!", (const char *) IPString);

	// Create the web server (task)
	new CZxWeb (&m_Net);

#endif	

	// Set up callback on the timer 100Hz interrupt
	// m_Timer.RegisterPeriodicHandler(PeriodicTimer100Hz);

	// Wait here forever until shutdown
	m_Event.Clear();
	m_Event.Wait();

	LOGNOTE("SHUTDOWN");

	return bReboot ? ShutdownReboot : ShutdownHalt;
}

// // Called once every 10ms
// void CKernel::AnimationFrame ()
// {
// 	 m_ZxScreen.UpdateScreen();
// }

// Don't do this here as it takes up way too much time in the IRQ. 
// Instead use a task that sleeps and wakes
// void CKernel::PeriodicTimer100Hz() {
// 	pKernel->AnimationFrame();
// }

// void CKernel::Reboot (void* pContext)
// {
// 	// CKernel *pThis = (CKernel *) pContext;

// 	// Performs a reboot, by exiting the main loop
// 	bReboot = TRUE;
// 	bRunning = FALSE;

// 	// Signal main loop
// 	pKernel->m_Event.Set();
// }

