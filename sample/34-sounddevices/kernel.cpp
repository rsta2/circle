//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2017  R. Stange <rsta2@o2online.de>
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
#include "config.h"
#include <circle/pwmsoundbasedevice.h>
#include <circle/i2ssoundbasedevice.h>
#include <circle/util.h>
#include <assert.h>

#ifdef USE_VCHIQ_SOUND
	#include <vc4/sound/vchiqsoundbasedevice.h>
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
#endif

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
#ifdef USE_VCHIQ_SOUND
	m_VCHIQ (&m_Memory, &m_Interrupt),
#endif
	m_pSound (0),
	m_VFO (&m_LFO),		// LFO modulates the VFO
	m_bNeedData (TRUE)
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

#ifdef USE_VCHIQ_SOUND
	if (bOK)
	{
		bOK = m_VCHIQ.Initialize ();
	}
#endif

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	// select the sound device
	const char *pSoundDevice = m_Options.GetSoundDevice ();
	if (strcmp (pSoundDevice, "sndpwm") == 0)
	{
		m_pSound = new CPWMSoundBaseDevice (&m_Interrupt, SAMPLE_RATE, CHUNK_SIZE);
	}
	else if (strcmp (pSoundDevice, "sndi2s") == 0)
	{
		m_pSound = new CI2SSoundBaseDevice (&m_Interrupt, SAMPLE_RATE, CHUNK_SIZE);
	}
	else
	{
#ifdef USE_VCHIQ_SOUND
		m_pSound = new CVCHIQSoundBaseDevice (&m_VCHIQ, SAMPLE_RATE,
					(TVCHIQSoundDestination) m_Options.GetSoundOption ());
#else
		m_pSound = new CPWMSoundBaseDevice (&m_Interrupt, SAMPLE_RATE, CHUNK_SIZE);
#endif
	}
	assert (m_pSound != 0);

	// initialize oscillators
	m_LFO.SetWaveform (WaveformSine);
	m_LFO.SetFrequency (10.0);

	m_VFO.SetWaveform (WaveformSine);
	m_VFO.SetFrequency (440.0);
	m_VFO.SetModulationVolume (0.25);

	// initialize and start sound device
	m_pSound->SetWriteFormat (FORMAT, WRITE_CHANNELS);
	m_pSound->RegisterNeedDataCallback (NeedDataCallback, this);

	if (!m_pSound->Start ())
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot start sound device");
	}

	m_Logger.Write (FromKernel, LogNotice, "Playing modulated 440 Hz tone");

	// output sound data
	for (unsigned nCount = 0; m_pSound->IsActive (); nCount++)
	{
		if (m_bNeedData)
		{
			m_bNeedData = FALSE;

			u8 Buffer[CHUNK_SIZE * TYPE_SIZE];
			for (unsigned i = 0; i < CHUNK_SIZE;)
			{
				m_LFO.NextSample ();
				m_VFO.NextSample ();

				float fLevel = m_VFO.GetOutputLevel ();
				TYPE nLevel = (TYPE) (fLevel*VOLUME * FACTOR + NULL_LEVEL);

				memcpy (&Buffer[i++ * TYPE_SIZE], &nLevel, TYPE_SIZE);
#if WRITE_CHANNELS == 2
				memcpy (&Buffer[i++ * TYPE_SIZE], &nLevel, TYPE_SIZE);
#endif
			}

			int nResult = m_pSound->Write (Buffer, sizeof Buffer);
			if (nResult != sizeof Buffer)
			{
				m_Logger.Write (FromKernel, LogWarning, "Sound data dropped");
			}
		}

		m_Screen.Rotor (0, nCount);

#ifdef USE_VCHIQ_SOUND
		m_Scheduler.Yield ();
#endif
	}

	return ShutdownHalt;
}

void CKernel::NeedDataCallback (void *pParam)
{
	CKernel *pThis = (CKernel *) pParam;
	assert (pThis != 0);

	pThis->m_bNeedData = TRUE;
}
