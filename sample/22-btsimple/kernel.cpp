//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2016  R. Stange <rsta2@o2online.de>
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
#include "kernel.h"
#include <assert.h>

#define INQUIRY_SECONDS		20

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_DWHCI (&m_Interrupt, &m_Timer),
	m_Bluetooth (&m_Interrupt)
{
	m_ActLED.Blink (5);	// show we are alive
}

CKernel::~CKernel (void)
{
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
		bOK = m_DWHCI.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Bluetooth.Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	m_Logger.Write (FromKernel, LogNotice, "Inquiry is running for %u seconds", INQUIRY_SECONDS);

	CBTInquiryResults *pInquiryResults = m_Bluetooth.Inquiry (INQUIRY_SECONDS);
	if (pInquiryResults == 0)
	{
		m_Logger.Write (FromKernel, LogPanic, "Inquiry failed");
	}

	m_Logger.Write (FromKernel, LogNotice, "Inquiry complete, %u device(s) found",
			pInquiryResults->GetCount ());

	if (pInquiryResults->GetCount () > 0)
	{
		m_Logger.Write (FromKernel, LogNotice, "# BD address        Class  Name");

		for (unsigned nDevice = 0; nDevice < pInquiryResults->GetCount (); nDevice++)
		{
			const u8 *pBDAddress = pInquiryResults->GetBDAddress (nDevice);
			assert (pBDAddress != 0);
			
			const u8 *pClassOfDevice = pInquiryResults->GetClassOfDevice (nDevice);
			assert (pClassOfDevice != 0);
			
			m_Logger.Write (FromKernel, LogNotice, "%u %02X:%02X:%02X:%02X:%02X:%02X %02X%02X%02X %s",
					nDevice+1,
					(unsigned) pBDAddress[5],
					(unsigned) pBDAddress[4],
					(unsigned) pBDAddress[3],
					(unsigned) pBDAddress[2],
					(unsigned) pBDAddress[1],
					(unsigned) pBDAddress[0],
					(unsigned) pClassOfDevice[2],
					(unsigned) pClassOfDevice[1],
					(unsigned) pClassOfDevice[0],
					pInquiryResults->GetRemoteName (nDevice));
		}
	}

	delete pInquiryResults;

	return ShutdownHalt;
}
