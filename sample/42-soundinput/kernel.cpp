//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2023  R. Stange <rsta2@o2online.de>
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
#include <circle/util.h>
#include <assert.h>

#ifdef ENABLE_RECORDER
	#include "soundrecorder.h"
#endif

#if WRITE_FORMAT == 0
	#define FORMAT		SoundFormatUnsigned8
	#define TYPE		u8
	#define TYPE_SIZE	sizeof (u8)
	#define FACTOR		((1 << 7)-1)
	#define NULL_LEVEL	(1 << 7)
#elif WRITE_FORMAT == 1
	#define FORMAT		SoundFormatSigned16
	#define TYPE		s16
	#define TYPE_SIZE	sizeof (s16)
	#define FACTOR		((1 << 15)-1)
	#define NULL_LEVEL	0
#elif WRITE_FORMAT == 2
	#define FORMAT		SoundFormatSigned24
	#define TYPE		s32
	#define TYPE_SIZE	(sizeof (u8)*3)
	#define FACTOR		((1 << 23)-1)
	#define NULL_LEVEL	0
#elif WRITE_FORMAT == 3
	#define FORMAT		SoundFormatSigned24_32
	#define TYPE		s32
	#define TYPE_SIZE	(sizeof (s32))
	#define FACTOR		((1 << 23)-1)
	#define NULL_LEVEL	0
#endif

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
#ifdef USE_USB
	m_USBHCI (&m_Interrupt, &m_Timer, FALSE),
#endif
#ifdef ENABLE_RECORDER
	m_EMMC (&m_Interrupt, &m_Timer, &m_ActLED),
#endif
	m_pSoundIn (0),
	m_pSoundOut (0)
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

#ifdef USE_USB
	if (bOK)
	{
		bOK = m_USBHCI.Initialize ();
	}
#endif

#ifdef ENABLE_RECORDER
	if (bOK)
	{
		bOK = m_EMMC.Initialize ();
	}
#endif

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

#ifdef ENABLE_RECORDER
	// mount file system
	if (f_mount (&m_FileSystem, DRIVE, 1) != FR_OK)
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot mount drive: %s", DRIVE);
	}

	// start sound recorder task
	CSoundRecorder *pRecorder = new CSoundRecorder (&m_FileSystem);
	assert (pRecorder != 0);
#endif

	// create sound devices
#ifndef USE_USB
	m_pSoundIn = new CI2SSoundBaseDevice (&m_Interrupt, SAMPLE_RATE, CHUNK_SIZE, TRUE, 0, 0,
					      CI2SSoundBaseDevice::DeviceModeRXOnly);
#else
	m_pSoundIn = new CUSBSoundBaseDevice (SAMPLE_RATE, CUSBSoundBaseDevice::DeviceModeRXOnly);
#endif
	assert (m_pSoundIn != 0);

	m_pSoundOut = new CPWMSoundBaseDevice (&m_Interrupt, SAMPLE_RATE, CHUNK_SIZE);
	assert (m_pSoundOut != 0);

	// configure sound devices
	if (!m_pSoundIn->AllocateReadQueue (QUEUE_SIZE_MSECS))
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot allocate input sound queue");
	}

	m_pSoundIn->SetReadFormat (FORMAT, WRITE_CHANNELS);

	if (!m_pSoundOut->AllocateQueue (QUEUE_SIZE_MSECS))
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot allocate output sound queue");
	}

	m_pSoundOut->SetWriteFormat (FORMAT, WRITE_CHANNELS);

	// start sound devices
	if (!m_pSoundIn->Start ())
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot start input sound device");
	}

	if (!m_pSoundOut->Start ())
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot start output sound device");
	}

#ifdef INPUT_JACK
	// enable input jack and set volume
	CSoundController *pController = m_pSoundIn->GetController ();
	if (pController)
	{
		pController->EnableJack (INPUT_JACK);

#ifdef INPUT_VOLUME
		CSoundController::TControlInfo Info =
			pController->GetControlInfo (CSoundController::ControlVolume, INPUT_JACK,
						     CSoundController::ChannelAll);

		if (Info.Supported)
		{
			pController->SetControl (CSoundController::ControlVolume, INPUT_JACK,
						 CSoundController::ChannelAll, INPUT_VOLUME);
		}
#endif
	}
#endif

	// copy sound data
	for (unsigned nCount = 0; 1; nCount++)
	{
		u8 Buffer[TYPE_SIZE*WRITE_CHANNELS*4096];
		int nBytes = m_pSoundIn->Read (Buffer, sizeof Buffer);
		if (nBytes > 0)
		{
			int nResult = m_pSoundOut->Write (Buffer, nBytes);
			if (nResult != nBytes)
			{
				m_Logger.Write (FromKernel, LogWarning, "Sound data dropped");
			}

#ifdef ENABLE_RECORDER
			nResult = pRecorder->Write (Buffer, nBytes);
			if (nResult != nBytes)
			{
				m_Logger.Write (FromKernel, LogWarning,
						"Sound data dropped (recorder)");
			}
#endif
		}

		m_Screen.Rotor (0, nCount);

		m_Scheduler.Yield ();
	}

	return ShutdownHalt;
}
