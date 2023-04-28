//
// kernel.cpp
//
#include "kernel.h"

LOGMODULE ("kernel");

static volatile boolean bRunning = FALSE;
static volatile boolean bReboot = FALSE;



CKernel::CKernel (void)
:	// m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	// m_Serial (&m_Interrupt),
	m_Timer (&m_Interrupt),	
	m_Logger (m_Options.GetLogLevel (), &m_Timer),	
	// TODO: add more member initializers here
	m_Shell (&m_Serial),
	m_ZxSmi(&m_Interrupt),
	// m_ZxScreen(m_Options.GetWidth (), m_Options.GetHeight (), FALSE, 0, &m_Interrupt)
	// 320x256 (32 + 256 + 32 x 32 + 192 + 32) - border is 1/3rd screen (this is about right for original speccy)
	m_ZxScreen(320, 256, FALSE, 0, &m_Interrupt)
{
	bRunning = TRUE;
	bReboot = TRUE;


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
		m_ZxSmi.SetActLED(&m_ActLED);
	}

	if (bOK)
	{
		// bOK = m_Serial.Initialize (921600);
		bOK = m_Serial.Initialize (115200);
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
		bOK = m_ZxSmi.Initialize ();
		m_ZxSmi.SetActLED(&m_ActLED);
	}


	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	LOGNOTE("Compile time: " __DATE__ " " __TIME__);

	// Set the reboot callback 
	m_Shell.SetRebootCallback(Reboot, this);

	// TODO: add your code here
	m_ZxSmi.Start();

	m_ZxScreen.Start();

	// TScreenStatus screenStatus = m_Screen.GetStatus();
	// screenStatus.BackgroundColor = BLUE_COLOR;
	// m_Screen.SetStatus(screenStatus);
	// m_Screen.ClearDisplayEnd();
	// unsigned x = 0, y = 0;
	// m_Screen.SetPixel(0,0, BRIGHT_GREEN_COLOR);

	// for (x = 0; x < m_Screen.GetWidth(); x++) {
	// 	for (y = 0; y < m_Screen.GetHeight(); y++) {
	// 		m_Screen.SetPixel(x,y, BRIGHT_GREEN_COLOR);
	// 	}
	// }
	// x = 0;
	// y = 0;
	boolean clear = FALSE;

	while (bRunning) {				 
		// m_ZxSmi.Start();
		ZX_DMA_T value = m_ZxSmi.GetValue();
		// LOGDBG("DATA: %04lx", value);
		// m_Timer.MsDelay(500);

		// Read the shell (serial port)
		m_Shell.Update();

		// m_Screen.SetPixel(x++,y++, RED_COLOR);
		value = value & 0x7;
		TScreenColor bc = BLACK_COLOR;
		if (value == 1) bc = BLUE_COLOR;
		if (value == 2) bc = RED_COLOR;
		if (value == 3) bc = MAGENTA_COLOR;
		if (value == 4) bc = GREEN_COLOR;
		if (value == 5) bc = CYAN_COLOR;
		if (value == 6) bc = YELLOW_COLOR;
		if (value == 7) bc = WHITE_COLOR;

		m_ZxScreen.SetBorder(bc);
		m_ZxScreen.SetScreen(clear);

		clear = !clear;

		// for (x = 0; x < m_ZxScreen.GetWidth(); x++) {
		// for (y = 0; y < m_ZxScreen.GetHeight(); y++) {
		// 	m_Screen.SetPixel(x,y, bc);
		// }
		// }
	}

	LOGNOTE("SHUTDOWN");

	return bReboot ? ShutdownReboot : ShutdownHalt;
}

void CKernel::Reboot (void* pContext)
{
	// CKernel *pThis = (CKernel *) pContext;

	// Performs a reboot, by exiting the main loop
	bReboot = TRUE;
	bRunning = FALSE;
}

