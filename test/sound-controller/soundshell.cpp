//
// soundshell.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2023  R. Stange <rsta2@o2online.de>
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
#include "soundshell.h"
#include <circle/sound/pwmsoundbasedevice.h>
#include <circle/sound/i2ssoundbasedevice.h>
#include <circle/sound/hdmisoundbasedevice.h>
#include <circle/sound/usbsoundbasedevice.h>
#include <circle/sched/scheduler.h>
#include <circle/timer.h>
#include <circle/stdarg.h>
#include <circle/util.h>
#include <assert.h>

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

const char CSoundShell::HelpMsg[] =
	"\n"
	"Command\t\tDescription\t\t\t\t\t\tAlias\n"
	"\n"
	"start DEV [MODE] [CLK] [I2C]\n"
	"\t\tStart sound device (snd{pwm|i2s|hdmi"
#if RASPPI >= 4
	"|usb"
#endif
#ifdef USE_VCHIQ_SOUND
	"|vchiq"
#endif
	"})\n"
	"\t\t[for input (i), output (o, default) or both (io)]\n"
	"\t\t[with slave (s, default) or master (m) I2S clock]\n"
	"\t\t[and I2C slave address of DAC]\n"
	"cancel\t\tStop active sound device\t\t\t\tstop\n"
	"mode MODE\tNext commands effects input (i) or output (o) device\n"
	"enable JACK\tEnable jack (defaultout|lineout|speaker|headphone|hdmi\n"
	"\t\t|spdif|defaultin|linein|microphone)\n"
	"disable JACK\tDisable jack (multi-jack operation only)\n"
	"controlinfo CTRL CHAN [JACK]\t\t\t\t\t\tinfo\n"
	"\t\tDisplay control (mute|volume) info\n"
	"\t\tfor channel (all|l|r|NUM) [and jack]\n"
	"setcontrol CTRL CHAN VAL [JACK]\t\t\t\t\t\tset\n"
	"\t\tSet control for channel [and jack] to value\n"
	"oscillator WAVE [FREQ] [CHAN]\t\t\t\t\t\tosc\n"
	"\t\tSet oscillator parameters (sine|square|sawtooth|triangle\n"
	"\t\t|pulse12|pulse25|noise) [and frequency] [for channel]\n"
	"vumeter\t\tToggle VU meter display on screen\n"
	"delay MS\tDelay MS milliseconds\n"
	"reboot\t\tReboot the system\n"
	"help\t\tThis help\n"
	"\n";

const CSoundShell::TStringMapping CSoundShell::s_ModeMap[] =
{
	{"i",		ModeInput},
	{"o",		ModeOutput},
	{"io",		ModeInputOutput},
	{0,		ModeUnknown}
};

const CSoundShell::TStringMapping CSoundShell::s_ClockModeMap[] =
{
	{"m",		ClockModeMaster},
	{"s",		ClockModeSlave},
	{0,		ClockModeUnknown}
};

const CSoundShell::TStringMapping CSoundShell::s_JackMap[] =
{
	{"defaultout",	CSoundController::JackDefaultOut},
	{"lineout",	CSoundController::JackLineOut},
	{"speaker",	CSoundController::JackSpeaker},
	{"headphone",	CSoundController::JackHeadphone},
	{"hdmi",	CSoundController::JackHDMI},
	{"spdif",	CSoundController::JackSPDIFOut},
	{"defaultin",	CSoundController::JackDefaultIn},
	{"linein",	CSoundController::JackLineIn},
	{"microphone",	CSoundController::JackMicrophone},
	{0,		CSoundController::JackUnknown}
};

const CSoundShell::TStringMapping CSoundShell::s_ChannelMap[] =
{
	{"all",		CSoundController::ChannelAll},
	{"l",		CSoundController::ChannelLeft},
	{"r",		CSoundController::ChannelRight},
	{0,		CSoundController::ChannelUnknown}
};

const CSoundShell::TStringMapping CSoundShell::s_ControlMap[] =
{
	{"mute",	CSoundController::ControlMute},
	{"volume",	CSoundController::ControlVolume},
	{0,		CSoundController::ControlUnknown}
};

const CSoundShell::TStringMapping CSoundShell::s_WaveformMap[] =
{
	{"sine",	WaveformSine},
	{"square",	WaveformSquare},
	{"sawtooth",	WaveformSawtooth},
	{"triangle",	WaveformTriangle},
	{"pulse12",	WaveformPulse12},
	{"pulse25",	WaveformPulse25},
	{"noise",	WaveformNoise},
	{0,		WaveformUnknown}
};

CSoundShell::CSoundShell (CConsole *pConsole, CInterruptSystem *pInterrupt,
			  CI2CMaster *pI2CMaster,
#ifdef USE_VCHIQ_SOUND
			  CVCHIQDevice *pVCHIQ,
#endif
			  CScreenDevice *pScreen, CUSBHCIDevice *pUSBHCI)
:	m_pConsole (pConsole),
	m_pInterrupt (pInterrupt),
	m_pI2CMaster (pI2CMaster),
#ifdef USE_VCHIQ_SOUND
	m_pVCHIQ (pVCHIQ),
#endif
	m_pScreen (pScreen),
	m_pUSBHCI (pUSBHCI),
	m_bContinue (TRUE),
	m_nMode (ModeOutput),
	m_bI2SDevice {FALSE, FALSE},
	m_pSound {0, 0},
	m_nLastTicks (0)
{
	for (unsigned i = 0; i < WRITE_CHANNELS; i++)
	{
		m_pVUMeter[i] = 0;

		// initialize oscillators
		m_VCO[i].SetWaveform (WaveformSine);
		m_VCO[i].SetFrequency (440.0);
	}
}

CSoundShell::~CSoundShell (void)
{
	m_pConsole = 0;
	m_pInterrupt = 0;
	m_pI2CMaster = 0;
	m_pUSBHCI = 0;
	m_pSound[ModeOutput] = 0;
	m_pSound[ModeInput] = 0;
}

void CSoundShell::Run (void)
{
	Print ("\n\nSound Shell\n"
	       "Enter \"help\" for help!\n\n");

	while (m_bContinue)
	{
		ReadLine ();

		CString Command;
		while (GetToken (&Command))
		{
			if (((const char *) Command)[0] == '#')
			{
				break;
			}
			else if (Command.Compare ("start") == 0)
			{
				if (!Start ())
				{
					break;
				}
			}
			else if (   Command.Compare ("cancel") == 0
				 || Command.Compare ("stop") == 0)
			{
				if (!Cancel ())
				{
					break;
				}
			}
			else if (Command.Compare ("mode") == 0)
			{
				if (!Mode ())
				{
					break;
				}
			}
			else if (Command.Compare ("enable") == 0)
			{
				if (!Enable ())
				{
					break;
				}
			}
			else if (Command.Compare ("disable") == 0)
			{
				if (!Disable ())
				{
					break;
				}
			}
			else if (   Command.Compare ("controlinfo") == 0
				 || Command.Compare ("info") == 0)
			{
				if (!ControlInfo ())
				{
					break;
				}
			}
			else if (   Command.Compare ("setcontrol") == 0
				 || Command.Compare ("set") == 0)
			{
				if (!SetControl ())
				{
					break;
				}
			}
			else if (   Command.Compare ("oscillator") == 0
				 || Command.Compare ("osc") == 0)
			{
				if (!Oscillator ())
				{
					break;
				}
			}
			else if (Command.Compare ("vumeter") == 0)
			{
				if (!VUMeter ())
				{
					break;
				}
			}
			else if (Command.Compare ("delay") == 0)
			{
				if (!Delay ())
				{
					break;
				}
			}
			else if (Command.Compare ("reboot") == 0)
			{
				m_bContinue = FALSE;
			}
			else if (Command.Compare ("help") == 0)
			{
				if (!Help ())
				{
					break;
				}
			}
			else
			{
				Print ("Unknown command: %s\n", (const char *) Command);
				break;
			}
		}
	}
}

boolean CSoundShell::Start (void)
{
	CString Token;
	if (!GetToken (&Token))
	{
		Print ("Device name expected\n");

		return FALSE;
	}

	CString Mode;
	if (!GetToken (&Mode))
	{
		Mode = "o";
	}

	unsigned nMode = ConvertString (Mode, s_ModeMap);
	if (nMode == ModeUnknown)
	{
		Print ("Invalid device mode: %s\n", (const char *) Mode);

		return FALSE;
	}

	if (nMode != ModeInputOutput)
	{
		if (m_pSound[nMode] != 0)
		{
			Print ("Device already active\n");

			return FALSE;
		}
	}
	else
	{
		if (   m_pSound[ModeInput] != 0
		    || m_pSound[ModeOutput] != 0)
		{
			Print ("Device already active\n");

			return FALSE;
		}
	}

	// select the sound device
	assert (m_pInterrupt != 0);
	if (strcmp (Token, "sndpwm") == 0)
	{
		if (nMode != ModeOutput)
		{
			Print ("Device mode is not supported\n");

			return FALSE;
		}

		m_pSound[nMode] = new CPWMSoundBaseDevice (m_pInterrupt, SAMPLE_RATE, CHUNK_SIZE);
	}
	else if (strcmp (Token, "sndi2s") == 0)
	{
		CString ClockMode;
		if (!GetToken (&ClockMode))
		{
			ClockMode = "s";
		}

		unsigned nClockMode = ConvertString (ClockMode, s_ClockModeMap);
		if (nClockMode == ClockModeUnknown)
		{
			Print ("Invalid clock mode: %s\n", (const char *) ClockMode);

			return FALSE;
		}

		int nI2CSlaveAddress = GetNumber ("I2C slave address", 0, 0x7F, TRUE);
		if (nI2CSlaveAddress == INVALID_NUMBER)
		{
			nI2CSlaveAddress = 0;
		}

		assert (m_pI2CMaster != 0);

		if (   m_bI2SDevice[ModeOutput]
		    || m_bI2SDevice[ModeInput])
		{
			Print ("Device already active\n");

			return FALSE;
		}

		switch (nMode)
		{
		case ModeOutput:
			m_pSound[ModeOutput] = new CI2SSoundBaseDevice (
							m_pInterrupt,
							SAMPLE_RATE, CHUNK_SIZE,
							nClockMode == ClockModeSlave,
							m_pI2CMaster, nI2CSlaveAddress,
							CI2SSoundBaseDevice::DeviceModeTXOnly);
			m_bI2SDevice[ModeOutput] = TRUE;
			break;

		case ModeInput:
			m_pSound[ModeInput] = new CI2SSoundBaseDevice (
							m_pInterrupt,
							SAMPLE_RATE, CHUNK_SIZE,
							nClockMode == ClockModeSlave,
							m_pI2CMaster, nI2CSlaveAddress,
							CI2SSoundBaseDevice::DeviceModeRXOnly);
			m_bI2SDevice[ModeInput] = TRUE;
			break;

		case ModeInputOutput:
			m_pSound[ModeOutput] =
			m_pSound[ModeInput] = new CI2SSoundBaseDevice (
							m_pInterrupt,
							SAMPLE_RATE, CHUNK_SIZE,
							nClockMode == ClockModeSlave,
							m_pI2CMaster, nI2CSlaveAddress,
							CI2SSoundBaseDevice::DeviceModeTXRX);
			m_bI2SDevice[ModeOutput] = TRUE;
			m_bI2SDevice[ModeInput] = TRUE;
			break;

		default:
			assert (0);
			break;
		}
	}
	else if (strcmp (Token, "sndhdmi") == 0)
	{
		if (nMode != ModeOutput)
		{
			Print ("Device mode is not supported\n");

			return FALSE;
		}

		m_pSound[nMode] = new CHDMISoundBaseDevice (m_pInterrupt, SAMPLE_RATE, CHUNK_SIZE);
	}
#if RASPPI >= 4
	else if (strcmp (Token, "sndusb") == 0)
	{
		switch (nMode)
		{
		case ModeOutput:
			m_pSound[ModeOutput] = new CUSBSoundBaseDevice (
							SAMPLE_RATE,
							CUSBSoundBaseDevice::DeviceModeTXOnly);
			break;

		case ModeInput:
			m_pSound[ModeInput] = new CUSBSoundBaseDevice (
							SAMPLE_RATE,
							CUSBSoundBaseDevice::DeviceModeRXOnly);
			break;

		case ModeInputOutput:
			m_pSound[ModeOutput] =
			m_pSound[ModeInput] = new CUSBSoundBaseDevice (
							SAMPLE_RATE,
							CUSBSoundBaseDevice::DeviceModeTXRX);
			break;

		default:
			assert (0);
			break;
		}
	}
#endif
#ifdef USE_VCHIQ_SOUND
	else if (strcmp (Token, "sndvchiq") == 0)
	{
		if (nMode != ModeOutput)
		{
			Print ("Device mode is not supported\n");

			return FALSE;
		}

		assert (m_pVCHIQ != 0);
		m_pSound[nMode] = new CVCHIQSoundBaseDevice (m_pVCHIQ, SAMPLE_RATE, CHUNK_SIZE);
	}
#endif
	else
	{
		Print ("Invalid device name: %s\n", (const char *) Token);

		return FALSE;
	}

	if (nMode != ModeInput)
	{
		// configure sound device
		if (!m_pSound[ModeOutput]->AllocateQueue (QUEUE_SIZE_MSECS))
		{
			Print ("Cannot allocate sound queue\n");

			delete m_pSound[ModeOutput];
			m_pSound[ModeOutput] = 0;
			m_bI2SDevice[ModeOutput] = FALSE;

			return FALSE;
		}

		m_pSound[ModeOutput]->SetWriteFormat (FORMAT, WRITE_CHANNELS);

		// initially fill the whole queue with data
		//WriteSoundData (m_pSound[ModeOutput]->GetQueueSizeFrames ());
	}

	if (nMode != ModeOutput)
	{
		// configure sound device
		if (!m_pSound[ModeInput]->AllocateReadQueue (QUEUE_SIZE_MSECS))
		{
			Print ("Cannot allocate sound queue\n");

			delete m_pSound[ModeInput];
			m_pSound[ModeInput] = 0;
			m_bI2SDevice[ModeInput] = FALSE;

			return FALSE;
		}

		m_pSound[ModeInput]->SetReadFormat (FORMAT, WRITE_CHANNELS);
	}

	// start sound device
	if (nMode == ModeInputOutput)
	{
		if (!m_pSound[ModeOutput]->Start ())
		{
			Print ("Cannot start sound device\n");

			delete m_pSound[ModeOutput];
			m_pSound[ModeOutput] =
			m_pSound[ModeInput] = 0;
			m_bI2SDevice[ModeOutput] = FALSE;
			m_bI2SDevice[ModeInput] = FALSE;

			return FALSE;
		}
	}
	else
	{
		if (!m_pSound[nMode]->Start ())
		{
			Print ("Cannot start sound device\n");

			delete m_pSound[nMode];
			m_pSound[nMode] = 0;
			m_bI2SDevice[nMode] = FALSE;

			return FALSE;
		}

		m_nMode = nMode;
	}

	return TRUE;
}

boolean CSoundShell::Cancel (void)
{
	if (m_pSound[ModeOutput] != m_pSound[ModeInput])
	{
		if (!m_pSound[m_nMode])
		{
			Print ("Sound device is not active\n");

			return FALSE;
		}

		m_pSound[m_nMode]->Cancel ();

		while (m_pSound[m_nMode]->IsActive ())
		{
#ifdef USE_VCHIQ_SOUND
			CScheduler::Get ()->Yield ();
#endif
		}

		delete m_pSound[m_nMode];
		m_pSound[m_nMode] = 0;
		m_bI2SDevice[m_nMode] = FALSE;
	}
	else
	{
		if (!m_pSound[ModeOutput])
		{
			Print ("Sound device is not active\n");

			return FALSE;
		}

		m_pSound[ModeOutput]->Cancel ();

		while (m_pSound[ModeOutput]->IsActive ())
		{
#ifdef USE_VCHIQ_SOUND
			CScheduler::Get ()->Yield ();
#endif
		}

		delete m_pSound[ModeOutput];
		m_pSound[ModeOutput] =
		m_pSound[ModeInput] = 0;
		m_bI2SDevice[ModeOutput] = FALSE;
		m_bI2SDevice[ModeInput] = FALSE;
	}

	return TRUE;
}

boolean CSoundShell::Mode (void)
{
	CString Mode;
	if (!GetToken (&Mode))
	{
		Print ("Device mode expected\n");

		return FALSE;
	}

	unsigned nMode = ConvertString (Mode, s_ModeMap);
	if (nMode >= ModeInputOutput)
	{
		Print ("Invalid device mode: %s\n", (const char *) Mode);

		return FALSE;
	}

	if (m_pSound[ModeOutput] == m_pSound[ModeInput])
	{
		Print ("Mode command has no effect\n");

		return FALSE;
	}

	m_nMode = nMode;

	return TRUE;
}

boolean CSoundShell::Enable (void)
{
	CString Token;
	if (!GetToken (&Token))
	{
		Print ("Jack name expected\n");

		return FALSE;
	}

	CSoundController::TJack Jack =
		(CSoundController::TJack) ConvertString (Token, s_JackMap);
	if (Jack == CSoundController::JackUnknown)
	{
		Print ("Unknown jack: %s\n", (const char *) Token);

		return FALSE;
	}

	if (!m_pSound[m_nMode])
	{
		Print ("Sound device is not active\n");

		return FALSE;
	}

	CSoundController *pController = m_pSound[m_nMode]->GetController ();
	if (pController == 0)
	{
		Print ("Sound controller is not supported\n");

		return FALSE;
	}

	if (!pController->EnableJack (Jack))
	{
		Print ("Cannot enable jack\n");

		return FALSE;
	}

	return TRUE;
}

boolean CSoundShell::Disable (void)
{
	CString Token;
	if (!GetToken (&Token))
	{
		Print ("Jack name expected\n");

		return FALSE;
	}

	CSoundController::TJack Jack =
		(CSoundController::TJack) ConvertString (Token, s_JackMap);
	if (Jack == CSoundController::JackUnknown)
	{
		Print ("Unknown jack: %s\n", (const char *) Token);

		return FALSE;
	}

	if (!m_pSound[m_nMode])
	{
		Print ("Sound device is not active\n");

		return FALSE;
	}

	CSoundController *pController = m_pSound[m_nMode]->GetController ();
	if (pController == 0)
	{
		Print ("Sound controller is not supported\n");

		return FALSE;
	}

	if (!pController->DisableJack (Jack))
	{
		Print ("Cannot disable jack\n");

		return FALSE;
	}

	return TRUE;
}

boolean CSoundShell::ControlInfo (void)
{
	if (   !m_pSound[m_nMode]
	    || !m_pSound[m_nMode]->IsActive ())
	{
		Print ("Sound device is not active\n");

		return FALSE;
	}

	CString Token;
	if (!GetToken (&Token))
	{
		Print ("Control name expected\n");

		return FALSE;
	}

	CSoundController::TControl Control =
		(CSoundController::TControl) ConvertString (Token, s_ControlMap);
	if (Control == CSoundController::ControlUnknown)
	{
		Print ("Unknown control: %s\n", (const char *) Token);

		return FALSE;
	}

	if (!GetToken (&Token))
	{
		Print ("Channel name expected\n");

		return FALSE;
	}

	CSoundController::TChannel Channel = ConvertChannel (Token);
	if (Channel == CSoundController::ChannelUnknown)
	{
		Print ("Unknown channel: %s\n", (const char *) Token);

		return FALSE;
	}

	CSoundController::TJack DefaultJack = CSoundController::JackDefaultOut;
	if (m_nMode == ModeInput)
	{
		DefaultJack = CSoundController::JackDefaultIn;
	}

	CSoundController::TJack Jack = DefaultJack;
	if (GetToken (&Token))
	{
		Jack = (CSoundController::TJack) ConvertString (Token, s_JackMap);
		if (Jack == CSoundController::JackUnknown)
		{
			Jack = DefaultJack;

			UnGetToken (Token);
		}
	}

	CSoundController *pController = m_pSound[m_nMode]->GetController ();
	if (pController == 0)
	{
		Print ("Sound controller is not supported\n");

		return FALSE;
	}

	CSoundController::TControlInfo Info = pController->GetControlInfo (Control, Jack, Channel);
	if (!Info.Supported)
	{
		Print ("Control is not supported\n");
	}
	else
	{
		Print ("Control is supported with range %d to %d\n", Info.RangeMin, Info.RangeMax);
	}

	return TRUE;
}

boolean CSoundShell::SetControl (void)
{
	if (   !m_pSound[m_nMode]
	    || !m_pSound[m_nMode]->IsActive ())
	{
		Print ("Sound device is not active\n");

		return FALSE;
	}

	CString Token;
	if (!GetToken (&Token))
	{
		Print ("Control name expected\n");

		return FALSE;
	}

	CSoundController::TControl Control =
		(CSoundController::TControl) ConvertString (Token, s_ControlMap);
	if (Control == CSoundController::ControlUnknown)
	{
		Print ("Unknown control: %s\n", (const char *) Token);

		return FALSE;
	}

	if (!GetToken (&Token))
	{
		Print ("Channel name expected\n");

		return FALSE;
	}

	CSoundController::TChannel Channel = ConvertChannel (Token);
	if (Channel == CSoundController::ChannelUnknown)
	{
		Print ("Unknown channel: %s\n", (const char *) Token);

		return FALSE;
	}

	if (!GetToken (&Token))
	{
		Print ("Value expected\n");

		return FALSE;
	}

	int nValue = ConvertNumber (Token);
	if (nValue == INVALID_NUMBER)
	{
		Print ("Invalid value: %s\n", (const char *) Token);

		return FALSE;
	}

	CSoundController::TJack DefaultJack = CSoundController::JackDefaultOut;
	if (m_nMode == ModeInput)
	{
		DefaultJack = CSoundController::JackDefaultIn;
	}

	CSoundController::TJack Jack = DefaultJack;
	if (GetToken (&Token))
	{
		Jack = (CSoundController::TJack) ConvertString (Token, s_JackMap);
		if (Jack == CSoundController::JackUnknown)
		{
			Jack = DefaultJack;

			UnGetToken (Token);
		}
	}

	CSoundController *pController = m_pSound[m_nMode]->GetController ();
	if (pController == 0)
	{
		Print ("Sound controller is not supported\n");

		return FALSE;
	}

	CSoundController::TControlInfo Info = pController->GetControlInfo (Control, Jack, Channel);
	if (!Info.Supported)
	{
		Print ("Control is not supported\n");

		return FALSE;
	}

	if (!(Info.RangeMin <= nValue && nValue <= Info.RangeMax))
	{
		Print ("Value is out of range: %d\n", nValue);

		return FALSE;
	}

	if (!pController->SetControl (Control, Jack, Channel, nValue))
	{
		Print ("Setting control value failed\n");

		return FALSE;
	}

	return TRUE;
}

boolean CSoundShell::Oscillator (void)
{
	CString Token;
	if (!GetToken (&Token))
	{
		Print ("Waveform name expected\n");

		return FALSE;
	}

	TWaveform Waveform = (TWaveform) ConvertString (Token, s_WaveformMap);
	if (Waveform == WaveformUnknown)
	{
		Print ("Unknown waveform: %s\n", (const char *) Token);

		return FALSE;
	}

	CSoundController::TChannel Channel = CSoundController::ChannelAll;
	int nFrequency = GetNumber ("Frequency", 5, 20000, TRUE);
	if (nFrequency != INVALID_NUMBER)
	{
		if (GetToken (&Token))
		{
			Channel = ConvertChannel (Token);
			if (Channel == CSoundController::ChannelUnknown)
			{
				Channel = CSoundController::ChannelAll;

				UnGetToken (Token);
			}
		}
	}
	else
	{
		nFrequency = 440;
	}

	if (Channel != CSoundController::ChannelAll)
	{
		m_VCO[Channel-1].SetWaveform (Waveform);
		m_VCO[Channel-1].SetFrequency ((float) nFrequency);
	}
	else
	{
		for (unsigned i = 0; i < WRITE_CHANNELS; i++)
		{
			m_VCO[i].SetWaveform (Waveform);
			m_VCO[i].SetFrequency ((float) nFrequency);
		}
	}

	return TRUE;
}

boolean CSoundShell::VUMeter (void)
{
	if (!m_pVUMeter[0])
	{
		assert (m_pScreen != 0);
		unsigned nWidth = m_pScreen->GetWidth () / 12;
		unsigned nHeight = m_pScreen->GetHeight () / 160;
		unsigned nPosX = m_pScreen->GetWidth () - nWidth;

		for (unsigned i = 0; i < WRITE_CHANNELS; i++)
		{
			m_pVUMeter[i] = new CVUMeter (m_pScreen, nPosX, i*nHeight,
						      nWidth, nHeight-1);
			assert (m_pVUMeter[i] != 0);
		}
	}
	else
	{
		for (unsigned i = 0; i < WRITE_CHANNELS; i++)
		{
			delete m_pVUMeter[i];
			m_pVUMeter[i] = 0;
		}
	}

	return TRUE;
}

boolean CSoundShell::Delay (void)
{
	int nMillis = GetNumber ("Delay", 1, 5000);
	if (nMillis == INVALID_NUMBER)
	{
		return FALSE;
	}

	unsigned nStartTicks = CTimer::Get ()->GetTicks ();
	while (CTimer::Get ()->GetTicks () - nStartTicks < (unsigned) MSEC2HZ (nMillis))
	{
		ProcessSound ();
	}

	return TRUE;
}

boolean CSoundShell::Help (void)
{
	Print (HelpMsg);

	return TRUE;
}

int CSoundShell::GetNumber (const char *pName, int nMinimum, int nMaximum, boolean bOptional)
{
	assert (pName != 0);

	CString Token;
	if (!GetToken (&Token))
	{
		if (!bOptional)
		{
			Print ("%s expected\n", pName);
		}

		return INVALID_NUMBER;
	}

	int nValue = ConvertNumber (Token);
	if (nValue == INVALID_NUMBER)
	{
		if (!bOptional)
		{
			Print ("Invalid number: %s\n", (const char *) Token);
		}
		else
		{
			UnGetToken (Token);
		}

		return INVALID_NUMBER;
	}

	assert (nMinimum < nMaximum);
	if (   nValue < nMinimum
	    || nValue > nMaximum)
	{
		Print ("%s out of range: %s\n", pName, (const char *) Token);
		return INVALID_NUMBER;
	}

	return nValue;
}

void CSoundShell::ReadLine (void)
{
	int nResult;

	do
	{
		const char *pMode = "IO";
		if (m_pSound[ModeOutput] != m_pSound[ModeInput])
		{
			pMode = m_nMode == ModeOutput ? "O" : "I";
		}

		Print ("Sound %s%s> ",
		       pMode, m_pSound[ModeOutput] ||  m_pSound[ModeInput] ? "!" : "");

		assert (m_pConsole != 0);
		while ((nResult = m_pConsole->Read (m_LineBuffer, sizeof m_LineBuffer-1)) <= 0)
		{
			if (m_pUSBHCI != 0)
			{
				boolean bUpdated = m_pUSBHCI->UpdatePlugAndPlay ();

				if (bUpdated)
				{
					m_pConsole->UpdatePlugAndPlay ();
				}
			}

			ProcessSound ();
		}

		assert (nResult < (int) sizeof m_LineBuffer);
		assert (m_LineBuffer[nResult-1] == '\n');
		m_LineBuffer[nResult-1] = '\0';
	}
	while (nResult <= 1);

	m_bFirstToken = TRUE;
	m_UnGetToken = "";
}

boolean CSoundShell::GetToken (CString *pString)
{
	assert (pString != 0);

	if (m_UnGetToken.GetLength () > 0)
	{
		*pString = m_UnGetToken;
		m_UnGetToken = "";

		return TRUE;
	}

	char *p = strtok_r (m_bFirstToken ? m_LineBuffer : 0, " \t\n", &m_pSavePtr);
	m_bFirstToken = FALSE;

	if (p == 0)
	{
		return FALSE;
	}

	*pString = p;

	return TRUE;
}

void CSoundShell::UnGetToken (const CString &rString)
{
	assert (!m_bFirstToken);

	m_UnGetToken = rString;
}

int CSoundShell::ConvertNumber (const CString &rString)
{
	const char *p = rString;

	int nSign = 1;
	if (p[0] == '-')
	{
		nSign = -1;
		p++;
	}

	int nBase = 10;
	if (    p[0] == '0'
	    && (p[1] == 'x' || p[1] == 'X'))
	{
		nBase = 16;
		p += 2;
	}

	char *pEndPtr = 0;
	unsigned long ulNumber = strtoul (p, &pEndPtr, nBase);
	if (    pEndPtr != 0
	    && *pEndPtr != '\0')
	{
		return INVALID_NUMBER;
	}

	if (ulNumber > 0x7FFFFFFFU)
	{
		return INVALID_NUMBER;
	}

	return nSign * (int) ulNumber;
}

unsigned CSoundShell::ConvertString (const CString &rString, const TStringMapping *pMap)
{
	assert (pMap != 0);

	while (pMap->pString)
	{
		if (rString.Compare (pMap->pString) == 0)
		{
			return pMap->nID;
		}

		pMap++;
	}

	return pMap->nID;
}

CSoundController::TChannel CSoundShell::ConvertChannel (const CString &rString)
{
	CSoundController::TChannel Channel =
		(CSoundController::TChannel) ConvertString (rString, s_ChannelMap);
	if (Channel == CSoundController::ChannelUnknown)
	{
		int nChannel = ConvertNumber (rString);
		if (   nChannel != INVALID_NUMBER
		    && nChannel <= WRITE_CHANNELS)
		{
			Channel = (CSoundController::TChannel) nChannel;
		}
	}

	return Channel;
}

void CSoundShell::Print (const char *pFormat, ...)
{
	assert (pFormat != 0);

	va_list var;
	va_start (var, pFormat);

	CString Message;
	Message.FormatV (pFormat, var);

	assert (m_pConsole != 0);
	m_pConsole->Write (Message, Message.GetLength ());

	va_end (var);
}

void CSoundShell::ProcessSound (void)
{
	boolean bInactive = FALSE;

	if (   m_pSound[ModeOutput] != 0
	    && m_pSound[ModeInput] == 0)
	{
		if (m_pSound[ModeOutput]->IsActive ())
		{
			// fill the whole queue free space with data
			WriteSoundData (  m_pSound[ModeOutput]->GetQueueSizeFrames ()
					- m_pSound[ModeOutput]->GetQueueFramesAvail ());
		}
		else
		{
			delete m_pSound[ModeOutput];
			m_pSound[ModeOutput] = 0;

			bInactive = TRUE;
		}
	}
	else if (   m_pSound[ModeOutput] == 0
		 && m_pSound[ModeInput] != 0)
	{
		if (m_pSound[ModeInput]->IsActive ())
		{
			ReadSoundData ();
		}
		else
		{
			delete m_pSound[ModeInput];
			m_pSound[ModeInput] = 0;

			bInactive = TRUE;
		}
	}
	else if (   m_pSound[ModeOutput] != 0
		 && m_pSound[ModeInput] != 0)
	{
		if (!m_pSound[ModeOutput]->IsActive ())
		{
			if (m_pSound[ModeOutput] != m_pSound[ModeInput])
			{
				delete m_pSound[ModeOutput];
			}

			m_pSound[ModeOutput] = 0;

			bInactive = TRUE;
		}

		if (!m_pSound[ModeInput]->IsActive ())
		{
			delete m_pSound[ModeInput];
			m_pSound[ModeInput] = 0;

			bInactive = TRUE;
		}

		if (   m_pSound[ModeOutput] != 0
		    && m_pSound[ModeInput] != 0)
		{
			ReadSoundData ();
		}
	}

	if (bInactive)
	{
		Print ("\nDevice(s) went inactive\n");
	}

	if (m_pVUMeter[0] != 0)
	{
		if (   m_pSound[0] == 0
		    && m_pSound[1] == 0)
		{
			for (unsigned i = 0; i < WRITE_CHANNELS; i++)
			{
				m_pVUMeter[i]->PutInputLevel (0.0f);
			}
		}

		unsigned nTicks = CTimer::Get ()->GetTicks ();
		if (nTicks - m_nLastTicks > HZ/50)
		{
			m_nLastTicks = nTicks;

			for (unsigned i = 0; i < WRITE_CHANNELS; i++)
			{
				m_pVUMeter[i]->Update ();
			}
		}
	}

#ifdef USE_VCHIQ_SOUND
	CScheduler::Get ()->Yield ();
#endif
}

void CSoundShell::WriteSoundData (unsigned nFrames)
{
	const unsigned nFramesPerWrite = 1000;
	u8 Buffer[nFramesPerWrite * WRITE_CHANNELS * TYPE_SIZE];

	while (nFrames > 0)
	{
		unsigned nWriteFrames = nFrames < nFramesPerWrite ? nFrames : nFramesPerWrite;

		GetSoundData (Buffer, nWriteFrames);

		unsigned nWriteBytes = nWriteFrames * WRITE_CHANNELS * TYPE_SIZE;

		int nResult = m_pSound[ModeOutput]->Write (Buffer, nWriteBytes);
		if (nResult != (int) nWriteBytes)
		{
			Print ("Sound data dropped\n");
		}

		nFrames -= nWriteFrames;

#ifdef USE_VCHIQ_SOUND
		CScheduler::Get ()->Yield ();		// ensure the VCHIQ tasks can run
#endif
	}
}

void CSoundShell::GetSoundData (void *pBuffer, unsigned nFrames)
{
	u8 *pBuffer8 = (u8 *) pBuffer;

	unsigned nSamples = nFrames * WRITE_CHANNELS;

	for (unsigned i = 0; i < nSamples;)
	{
		for (unsigned j = 0; j < WRITE_CHANNELS; j++)
		{
			m_VCO[j].NextSample ();

			float fLevel = m_VCO[j].GetOutputLevel () * VOLUME;
			TYPE nLevel = (TYPE) (fLevel * FACTOR + NULL_LEVEL);
			memcpy (&pBuffer8[i++ * TYPE_SIZE], &nLevel, TYPE_SIZE);

			if (m_pVUMeter[j] != 0)
			{
				m_pVUMeter[j]->PutInputLevel (fLevel);
			}
		}
	}
}

void CSoundShell::ReadSoundData (void)
{
	u8 Buffer[TYPE_SIZE*WRITE_CHANNELS*1000];
	int nBytes = m_pSound[ModeInput]->Read (Buffer, sizeof Buffer);
	if (nBytes > 0)
	{
		if (m_pSound[ModeOutput] != 0)
		{
			m_pSound[ModeOutput]->Write (Buffer, nBytes);
		}

		if (m_pVUMeter[0] != 0)
		{
			unsigned nFrames = nBytes / (TYPE_SIZE*WRITE_CHANNELS);

			for (unsigned i = 0; i < nFrames; i++)
			{
				for (unsigned j = 0; j < WRITE_CHANNELS; j++)
				{
					TYPE nLevel = 0;
					memcpy (&nLevel, &Buffer[(i*WRITE_CHANNELS + j) * TYPE_SIZE],
						TYPE_SIZE);
#if WRITE_FORMAT == 2
					// sign extension for 24-bit values
					if (nLevel > 0x7FFFFF)
					{
						nLevel |= 0xFF000000U;
					}
#endif
					float fLevel = (float) (nLevel - NULL_LEVEL) / FACTOR;

					m_pVUMeter[j]->PutInputLevel (fLevel);
				}
			}
		}
	}
}
