//
// miniorgan.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017  R. Stange <rsta2@o2online.de>
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
#include "miniorgan.h"
#include <circle/devicenameservice.h>
#include <circle/usb/usbkeyboard.h>
#include <circle/logger.h>
#include <assert.h>

#define SAMPLE_RATE	44100

#define VOLUME_PERCENT	20

static const char FromMiniOrgan[] = "organ";

// See: https://en.wikipedia.org/wiki/Piano_key_frequencies
TNoteInfo CMiniOrgan::s_Keys[] =
{
	{'M', 493.883}, // B4
	{'J', 466.164}, // A#4
	{'N', 440.000}, // A4
	{'H', 415.305}, // G#4
	{'B', 391.995}, // G4
	{'G', 369.994}, // F#4
	{'V', 349.228}, // F4
	{'C', 329.628}, // E4
	{'D', 311.127}, // D#4
	{'X', 293.665}, // D4
	{'S', 277.183}, // C#4
	{'Z', 261.626}  // C4	(actually the 'Y' key on some keyboards)
};

CMiniOrgan *CMiniOrgan::s_pThis = 0;

CMiniOrgan::CMiniOrgan (CInterruptSystem *pInterrupt)
:	CPWMSoundDevice2 (pInterrupt, SAMPLE_RATE),
	m_nCurrentLevel (0),
	m_nSampleCount (0),
	m_nFrequency (0),
	m_nPrevFrequency (0)
{
	s_pThis = this;

	m_nHighLevel = (GetRange ()-1) * VOLUME_PERCENT / 100;
}

CMiniOrgan::~CMiniOrgan (void)
{
	s_pThis = 0;
}

boolean CMiniOrgan::Initialize (void)
{
	CUSBKeyboardDevice *pKeyboard =
		(CUSBKeyboardDevice *) CDeviceNameService::Get ()->GetDevice ("ukbd1", FALSE);
	if (pKeyboard == 0)
	{
		CLogger::Get ()->Write (FromMiniOrgan, LogError, "Keyboard not found");

		return FALSE;
	}

	pKeyboard->RegisterKeyStatusHandlerRaw (KeyStatusHandlerRaw);

	return TRUE;
}

unsigned CMiniOrgan::GetChunk (u32 *pBuffer, unsigned nChunkSize)
{
	unsigned nResult = nChunkSize;

	// reset sample counter if key has changed
	if (m_nFrequency != m_nPrevFrequency)
	{
		m_nSampleCount = 0;

		m_nPrevFrequency = m_nFrequency;
	}

	// output level has to be changed on every nSampleDelay'th sample (if key is pressed)
	unsigned nSampleDelay = 0;
	if (m_nFrequency != 0)
	{
		nSampleDelay = (SAMPLE_RATE + m_nFrequency/2) / m_nFrequency;
	}

	for (; nChunkSize > 0; nChunkSize -= 2)		// fill the whole buffer
	{
		if (m_nFrequency != 0)			// key pressed?
		{
			// change output level if required to generate a square wave
			if (++m_nSampleCount == nSampleDelay)
			{
				m_nSampleCount = 0;

				if (m_nCurrentLevel == 0)
				{
					m_nCurrentLevel = m_nHighLevel;
				}
				else
				{
					m_nCurrentLevel = 0;
				}
			}

			*pBuffer++ = m_nCurrentLevel;
			*pBuffer++ = m_nCurrentLevel;
		}
		else
		{
			*pBuffer++ = 0;
			*pBuffer++ = 0;
		}
	}

	return nResult;
}

void CMiniOrgan::KeyStatusHandlerRaw (unsigned char ucModifiers, const unsigned char RawKeys[6])
{
	assert (s_pThis != 0);

	// find the key code of a pressed key
	char chKey = '\0';
	for (unsigned i = 0; i <= 5; i++)
	{
		if (RawKeys[i] != 0)
		{
			chKey = RawKeys[0]-0x04+'A';	// key code of 'A' is 0x04

			break;
		}
	}

	if (chKey != '\0')
	{
		// find the pressed key in the key table and set its frequency
		for (unsigned i = 0; i < sizeof s_Keys / sizeof s_Keys[0]; i++)
		{
			if (s_Keys[i].Key == chKey)
			{
				s_pThis->m_nFrequency = (unsigned) (s_Keys[i].Frequency + 0.5);

				return;
			}
		}
	}

	s_pThis->m_nFrequency = 0;
}
