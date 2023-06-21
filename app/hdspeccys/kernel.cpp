//
// kernel.cpp
//
#include "kernel.h"

#if HD_SPECCYS_FEATURE_NETWORK
#include "webserver.h"
#endif // HD_SPECCYS_FEATURE_NETWORK

LOGMODULE ("kernel");

static volatile boolean bReboot = TRUE;
static CKernel *pKernel = 0;

#if HD_SPECCYS_FEATURE_NETWORK
#if HD_SPECCYS_DHCP
#else
static const u8 IPAddress[]      = {192, 168, 0, 250};
static const u8 NetMask[]        = {255, 255, 255, 0};
static const u8 DefaultGateway[] = {192, 168, 0, 1};
static const u8 DNSServer[]      = {192, 168, 0, 1};
#endif // HD_SPECCYS_DHCP
#endif // HD_SPECCYS_FEATURE_NETWORK



CKernel::CKernel (void)
:	// m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	// m_Serial (&m_Interrupt),
	m_Timer (&m_Interrupt),	
	m_Logger (m_Options.GetLogLevel(), &m_Timer),	
	// TODO: add more member initializers here
	m_GPIOManager(&m_Interrupt),
#if HD_SPECCYS_FEATURE_NETWORK
	m_USBHCI (&m_Interrupt, &m_Timer),
#if HD_SPECCYS_DHCP
#else
	m_Net (IPAddress, NetMask, DefaultGateway, DNSServer),
#endif
#endif	
	m_Shell (&m_Serial),
	m_ZxTape(&m_GPIOManager, &m_Interrupt),
	m_ZxSmi(&m_GPIOManager, &m_Interrupt),
	// m_ZxScreen(m_Options.GetWidth (), m_Options.GetHeight (), FALSE, 0, &m_Interrupt)
	// 320x256 (32 + 256 + 32 x 32 + 192 + 32) - border is 1/3rd screen (this is about right for original speccy)
	m_ZxScreen(320, 256, 0, &m_Interrupt)
	// m_ZxScreen(320*4, 256*4, 0, &m_Interrupt)
	//  m_ZxScreen (m_Options.GetWidth (), m_Options.GetHeight (), 0, &m_Interrupt)
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
		bOK = m_ZxScreen.Initialize ();
		m_ZxScreen.SetActLED(&m_ActLED);
	}

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

#if HD_SPECCYS_FEATURE_NETWORK
	if (bOK)
	{
		bOK = m_USBHCI.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Net.Initialize ();
	}
#endif	
	
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

#if HD_SPECCYS_FEATURE_NETWORK
	CString IPString;
	m_Net.GetConfig ()->GetIPAddress ()->Format (&IPString);
	LOGNOTE("Open \"http://%s/\" in your web browser!", (const char *) IPString);

	new CWebServer (&m_Net, &m_ActLED);

#endif	

	// Set up callback on the timer 100Hz interrupt
	// m_Timer.RegisterPeriodicHandler(PeriodicTimer100Hz);


	new CBackgroundTask (&m_Shell, &m_ActLED, &m_Event);
	new CScreenProcessorTask (&m_ZxScreen, &m_ZxSmi, &m_ActLED);
	new CTapeTask (&m_ZxTape);

	// Wait here forever until shutdown
	m_Event.Clear();
	m_Event.Wait();

	LOGNOTE("SHUTDOWN");

	return bReboot ? ShutdownReboot : ShutdownHalt;
}

// Called once every 10ms
void CKernel::AnimationFrame ()
{
	 m_ZxScreen.UpdateScreen();
}

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

