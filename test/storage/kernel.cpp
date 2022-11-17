//
// kernel.cpp
//

#include <circle/alloc.h>

#include "kernel.h"
#include "shasum.h"

//#define PLUG_AND_PLAY

#define LONESHA256_STATIC
#include "lonesha256.h"

#define DRIVE		"SD:"		// "USB:"
#define DEVICE		"emmc1"		// "umsd1"
#define FILENAME_READ	"/testfile.bin"
#define FILENAME_WRITE	"/testfile2.bin"

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_EMMC (&m_Interrupt, &m_Timer, &m_ActLED),
#ifdef PLUG_AND_PLAY
	m_USBHCI (&m_Interrupt, &m_Timer, TRUE),		// TRUE: enable plug-and-play
#else
	m_USBHCI (&m_Interrupt, &m_Timer, FALSE),
#endif

	m_pBuffer(new u8[400 * MEGABYTE]),
	m_nFileSize(0),

	m_bStorageAttached(FALSE)
{
	m_ActLED.Blink (5);	// show we are alive
}

CKernel::~CKernel (void)
{
	delete[] m_pBuffer;
}

boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;

	if (bOK)
	{
		bOK = m_Screen.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Serial.Initialize (115200);
	}

	if (bOK)
	{
		CDevice *pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
		if (pTarget == 0)
		{
			pTarget = &m_Screen;
		}

		bOK = m_Logger.Initialize (pTarget);
	}

	if (bOK)
	{
		bOK = m_Interrupt.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Timer.Initialize ();
	}

	if (bOK)
	{
		bOK = m_EMMC.Initialize ();
	}

	if (bOK)
	{
		bOK = m_USBHCI.Initialize ();
	}

	return bOK;
}

bool CKernel::ReadTest()
{
	FIL File;
	FRESULT Result = f_open (&File, DRIVE FILENAME_READ, FA_READ);
	if (Result != FR_OK)
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot open %s for reading", FILENAME_READ);
		return false;
	}

	m_nFileSize = f_size (&File);
	m_Logger.Write (FromKernel, LogNotice, "Reading %dMB from " DRIVE "...", m_nFileSize / 1024 / 1024);
	unsigned int nStartTicks = CTimer::Get()->GetClockTicks();

	UINT nBytesRead = 0;
	Result = f_read (&File, m_pBuffer, m_nFileSize, &nBytesRead);
	if (Result != FR_OK)
	{
		m_Logger.Write (FromKernel, LogPanic, "Failed to read %s into memory", FILENAME_READ);
		f_close (&File);
		return false;
	}

	f_close (&File);
	float nSeconds = (CTimer::Get()->GetClockTicks() - nStartTicks) / 1000000.0f;
	m_Logger.Write (FromKernel, LogNotice, "File read in %0.2f seconds", nSeconds);

	return m_nFileSize == nBytesRead;
}

bool CKernel::HashTest()
{
	m_Logger.Write (FromKernel, LogNotice, "Performing SHA256 hash...");
	u8 Hash[32]{0};
	lonesha256 (Hash, m_pBuffer, m_nFileSize);

	CString HexByte;
	CString HashString;
	for (u8 nByte : Hash)
	{
		HexByte.Format ("%02x", nByte);
		HashString.Append (HexByte);
	}

	m_Logger.Write (FromKernel, LogNotice, "SHA256 sum for %s: %s", FILENAME_READ, static_cast<const char*>(HashString));

	boolean bSuccess = HashString.Compare(ExpectedSHASum) == 0;
	if (bSuccess)
		m_Logger.Write (FromKernel, LogNotice, "SHA256 sum MATCH");
	else
		m_Logger.Write (FromKernel, LogError, "SHA256 sum FAIL");

	return bSuccess;
}

bool CKernel::WriteTest()
{
	FIL File;
	FRESULT Result = f_open (&File, DRIVE FILENAME_WRITE, FA_WRITE | FA_CREATE_ALWAYS);
	if (Result != FR_OK)
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot open %s for writing", FILENAME_WRITE);
		return false;
	}

	m_Logger.Write (FromKernel, LogNotice, "Writing %dMB to " DRIVE "...", m_nFileSize / 1024 / 1024);

	unsigned int nStartTicks = CTimer::Get()->GetClockTicks();

	UINT nBytesWritten = 0;
	Result = f_write (&File, m_pBuffer, m_nFileSize, &nBytesWritten);
	if (Result != FR_OK)
	{
		m_Logger.Write (FromKernel, LogPanic, "Failed to write %s", FILENAME_WRITE);
		f_close (&File);
		return false;
	}

	f_close (&File);
	float nSeconds = (CTimer::Get()->GetClockTicks() - nStartTicks) / 1000000.0f;
	m_Logger.Write (FromKernel, LogNotice, "File written in %0.2f seconds", nSeconds);

	return m_nFileSize == nBytesWritten;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	unsigned int nTestRuns = 0;
	unsigned int nSuccessfulTestRuns = 0;

#ifndef PLUG_AND_PLAY
	// Try to mount file system
	FRESULT Result = f_mount (&m_FileSystem, DRIVE, 1);
	if (Result != FR_OK)
	{
		m_Logger.Write (FromKernel, LogError, "Mount error (%u)", Result);
		return ShutdownHalt;
	}
#endif

	while (TRUE)
	{
		++nTestRuns;

#ifdef PLUG_AND_PLAY
		// Wait for stick
		m_Logger.Write (FromKernel, LogNotice, "Plug in an USB flash drive!");
		while (TRUE)
		{
			// Update the tree of connected USB devices
			if (m_USBHCI.UpdatePlugAndPlay ())
			{
				// Try to mount file system
				FRESULT Result = f_mount (&m_FileSystem, DRIVE, 1);
				if (Result == FR_OK)
				{
					break;
				}
				else if (Result != FR_NOT_READY)
				{
					m_Logger.Write (FromKernel, LogError,
							"Mount error (%u)", Result);
				}
			}
		}

		assert (!m_bStorageAttached);
		m_bStorageAttached = TRUE;
#endif

		CDevice *pDevice = m_DeviceNameService.GetDevice (DEVICE, TRUE);
		assert (pDevice != 0);
		pDevice->RegisterRemovedHandler (StorageRemovedHandler, this);

		if (ReadTest() && HashTest() && WriteTest())
		{
			++nSuccessfulTestRuns;
			m_Logger.Write (FromKernel, LogNotice, "=== TEST PASSED! (%d/%d) ===", nSuccessfulTestRuns, nTestRuns);
		}
		else
			m_Logger.Write (FromKernel, LogError, "=== TEST FAILED! (%d/%d) ===", nSuccessfulTestRuns, nTestRuns);

#ifdef PLUG_AND_PLAY
		// Wait for stick to be removed
		m_Logger.Write (FromKernel, LogNotice, "Remove USB flash drive!");
		while (TRUE)
		{
			// Update the tree of connected USB devices
			if (m_USBHCI.UpdatePlugAndPlay ())
			{
				if (!m_bStorageAttached)
				{
					break;
				}
			}
		}
#endif
	}

	return ShutdownHalt;
}

void CKernel::StorageRemovedHandler (CDevice *pDevice, void *pContext)
{
	CKernel *pThis = (CKernel *) pContext;
	assert (pThis != 0);

	assert (pThis->m_bStorageAttached);
	pThis->m_bStorageAttached = FALSE;
}
