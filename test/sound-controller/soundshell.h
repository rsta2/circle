//
// soundshell.h
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
#ifndef _soundshell_h
#define _soundshell_h

#include <circle/input/console.h>
#include <circle/usb/usbhcidevice.h>
#include <circle/sound/soundbasedevice.h>
#include <circle/interrupt.h>
#include <circle/i2cmaster.h>
#include <circle/screen.h>
#include <circle/string.h>
#include <circle/types.h>
#include "config.h"
#include "oscillator.h"
#include "vumeter.h"

#ifdef USE_VCHIQ_SOUND
	#include <vc4/sound/vchiqsoundbasedevice.h>
#endif

#define SOUNDSHELL_MAX_LINE	500

class CSoundShell
{
public:
	CSoundShell (CConsole *pConsole, CInterruptSystem *pInterrupt,
		     CI2CMaster *pI2CMaster,
#ifdef USE_VCHIQ_SOUND
		     CVCHIQDevice *pVCHIQ,
#endif
		     CScreenDevice *pScreen, CUSBHCIDevice *pUSBHCI = 0);
	~CSoundShell (void);

	void Run (void);

private:
	boolean Start (void);
	boolean Cancel (void);
	boolean Mode (void);
	boolean Enable (void);
	boolean Disable (void);
	boolean ControlInfo (void);
	boolean SetControl (void);
	boolean Oscillator (void);
	boolean VUMeter (void);
	boolean Delay (void);
	boolean Help (void);

	int GetNumber (const char *pName, int nMinimum, int nMaximum, boolean bOptional = FALSE);

private:
	void ReadLine (void);
	boolean GetToken (CString *pString);
	void UnGetToken (const CString &rString);

	static int ConvertNumber (const CString &rString);
#define INVALID_NUMBER	((int) 0x80000000)

	struct TStringMapping
	{
		const char *pString;
		unsigned nID;
	};
	static unsigned ConvertString (const CString &rString, const TStringMapping *pMap);

	CSoundController::TChannel ConvertChannel (const CString &rString);

	void Print (const char *pFormat, ...);

private:
	void ProcessSound (void);

	void WriteSoundData (unsigned nFrames);
	void GetSoundData (void *pBuffer, unsigned nFrames);

	void ReadSoundData (void);

	enum TMode
	{
		ModeOutput,
		ModeInput,
		ModeInputOutput,
		ModeUnknown
	};

	enum TClockMode
	{
		ClockModeMaster,
		ClockModeSlave,
		ClockModeUnknown
	};

private:
	CConsole *m_pConsole;
	CInterruptSystem *m_pInterrupt;
	CI2CMaster *m_pI2CMaster;
#ifdef USE_VCHIQ_SOUND
	CVCHIQDevice *m_pVCHIQ;
#endif
	CScreenDevice *m_pScreen;
	CUSBHCIDevice *m_pUSBHCI;

	boolean m_bContinue;

	unsigned m_nMode;
	boolean m_bI2SDevice[2];
	CSoundBaseDevice *m_pSound[2];
	COscillator m_VCO[WRITE_CHANNELS];

	CVUMeter *m_pVUMeter[WRITE_CHANNELS];
	unsigned m_nLastTicks;

	char m_LineBuffer[SOUNDSHELL_MAX_LINE+1];
	boolean m_bFirstToken;
	CString m_UnGetToken;
	char *m_pSavePtr;

	static const char HelpMsg[];

	static const TStringMapping s_ModeMap[];
	static const TStringMapping s_ClockModeMap[];
	static const TStringMapping s_JackMap[];
	static const TStringMapping s_ChannelMap[];
	static const TStringMapping s_ControlMap[];
	static const TStringMapping s_WaveformMap[];
};

#endif
