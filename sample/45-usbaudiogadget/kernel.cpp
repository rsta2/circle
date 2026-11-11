// SPDX-License-Identifier: GPL-3.0-or-later
//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
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
#include <circle/sysconfig.h>

LOGMODULE ("kernel");

static CUSBAudioGadgetFunction::TConfiguration GetConfiguration (void)
{
	CUSBAudioGadgetFunction::TConfiguration Config {};
	Config.nControlInterface = 0;
	Config.nStreamingInterface = 1;
	Config.nEndpoint = 1;
	Config.nInputTerminal = 1;
	Config.nFeatureUnit = 2;
	Config.nOutputTerminal = 3;
	Config.nChannels = 2;
	Config.wChannelConfig = 3;
	// Front left/right
	Config.nAlternates = 5;
	for (unsigned i = 0; i < Config.nAlternates; i++)
	{
		Config.Alternates[i].Format = (CUSBAudioGadgetFunction::TFormat) i;
		Config.Alternates[i].nRates = 3;
		Config.Alternates[i].Rates[0] = 32000;
		Config.Alternates[i].Rates[1] = 44100;
		Config.Alternates[i].Rates[2] = 48000;
	}
	return Config;
}

CAudioOutput::CAudioOutput (CInterruptSystem *pInterrupt, CUSBAudioGadgetBuffer *pBuffer)
:	CPWMSoundBaseDevice (pInterrupt, 48000, 256), m_pBuffer (pBuffer)
{
}

unsigned CAudioOutput::GetChunk (u32 *pBuffer, unsigned nWords)
{
	s32 Samples[64];
	unsigned nDone = 0;
	while (nDone < nWords)
	{
		unsigned n = nWords-nDone < 64 ? nWords-nDone : 64;
		m_pBuffer->ReadFrames (Samples, n/2);
		for (unsigned i = 0; i < n; i++)
		{
			u32 nUnsigned = (u32) Samples[i] ^ 0x80000000;
			pBuffer[nDone++] = (u32) ((u64) nUnsigned * (GetRangeMax ()+1) >> 32);
		}
	}
	return nWords;
}

CKernel::CKernel (void)
:	m_Timer (&m_Interrupt), m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_Buffer (48000, 2, 1024), m_Output (&m_Interrupt, &m_Buffer),
	// Development-only IDs following Circle's MIDI sample. Obtain IDs for distribution.
	m_Gadget (&m_Interrupt, GetConfiguration (), &m_Buffer,
		  USB_GADGET_VENDOR_ID, USB_GADGET_DEVICE_ID_AUDIO)
{
	m_ActLED.Blink (5);
}

boolean CKernel::Initialize (void)
{
	if (!m_Serial.Initialize (115200))
	{
		return FALSE;
	}
	return m_Logger.Initialize (&m_Serial) && m_Interrupt.Initialize ()
		&& m_Timer.Initialize () && m_Buffer.IsValid () && m_Gadget.Initialize ();
}

TShutdownMode CKernel::Run (void)
{
	LOGNOTE ("UAC1 adaptive playback: PWM stereo, 32/44.1/48 kHz");
	if (!m_Output.Start ())
	{
		return ShutdownHalt;
	}
	unsigned nLastTime = m_Timer.GetTime ();
	while (1)
	{
		m_Gadget.UpdatePlugAndPlay ();
		m_Gadget.Process ();
		unsigned nNow = m_Timer.GetTime ();
		if (nNow-nLastTime >= 5)
		{
			CUSBAudioGadgetFunction::TStatistics Statistics;
			m_Gadget.GetStatistics (&Statistics);
			LOGNOTE ("Packets %u malformed %u drops %u transitions %u failures %u",
				Statistics.nPackets, Statistics.nMalformed, Statistics.nDropped,
				Statistics.nTransitions, Statistics.nFailures);
			nLastTime = nNow;
		}
	}
	return ShutdownHalt;
}
