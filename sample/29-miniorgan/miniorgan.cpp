//
// miniorgan.cpp
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
#include "miniorgan.h"
#include <circle/devicenameservice.h>
#include <circle/sysconfig.h>
#include <circle/logger.h>
#include <assert.h>

#define VOLUME_PERCENT	20

#define MIDI_NOTE_OFF	0b1000
#define MIDI_NOTE_ON	0b1001
#define MIDI_CC		0b1011

#define MIDI_CC_VOLUME	7

#define KEY_NONE	255

static const char FromMiniOrgan[] = "organ";

// See: http://www.deimos.ca/notefreqs/
const float CMiniOrgan::s_KeyFrequency[/* MIDI key number */] =
{
	8.17580, 8.66196, 9.17702, 9.72272, 10.3009, 10.9134, 11.5623, 12.2499, 12.9783, 13.7500,
	14.5676, 15.4339, 16.3516, 17.3239, 18.3540, 19.4454, 20.6017, 21.8268, 23.1247, 24.4997,
	25.9565, 27.5000, 29.1352, 30.8677, 32.7032, 34.6478, 36.7081, 38.8909, 41.2034, 43.6535,
	46.2493, 48.9994, 51.9131, 55.0000, 58.2705, 61.7354, 65.4064, 69.2957, 73.4162, 77.7817,
	82.4069, 87.3071, 92.4986, 97.9989, 103.826, 110.000, 116.541, 123.471, 130.813, 138.591,
	146.832, 155.563, 164.814, 174.614, 184.997, 195.998, 207.652, 220.000, 233.082, 246.942,
	261.626, 277.183, 293.665, 311.127, 329.628, 349.228, 369.994, 391.995, 415.305, 440.000,
	466.164, 493.883, 523.251, 554.365, 587.330, 622.254, 659.255, 698.456, 739.989, 783.991,
	830.609, 880.000, 932.328, 987.767, 1046.50, 1108.73, 1174.66, 1244.51, 1318.51, 1396.91,
	1479.98, 1567.98, 1661.22, 1760.00, 1864.66, 1975.53, 2093.00, 2217.46, 2349.32, 2489.02,
	2637.02, 2793.83, 2959.96, 3135.96, 3322.44, 3520.00, 3729.31, 3951.07, 4186.01, 4434.92,
	4698.64, 4978.03, 5274.04, 5587.65, 5919.91, 6271.93, 6644.88, 7040.00, 7458.62, 7902.13,
	8372.02, 8869.84, 9397.27, 9956.06, 10548.1, 11175.3, 11839.8, 12543.9
};

const TNoteInfo CMiniOrgan::s_Keys[] =
{
	{',', 72}, // C4
	{'M', 71}, // B4
	{'J', 70}, // A#4
	{'N', 69}, // A4
	{'H', 68}, // G#3
	{'B', 67}, // G3
	{'G', 66}, // F#3
	{'V', 65}, // F3
	{'C', 64}, // E3
	{'D', 63}, // D#3
	{'X', 62}, // D3
	{'S', 61}, // C#3
	{'Z', 60}  // C3
};

CMiniOrgan *CMiniOrgan::s_pThis = 0;

CMiniOrgan::CMiniOrgan (CInterruptSystem *pInterrupt, CI2CMaster *pI2CMaster)
#ifdef USE_USB
:	SOUND_CLASS (SAMPLE_RATE
#else
:	SOUND_CLASS (pInterrupt, SAMPLE_RATE, CHUNK_SIZE
#ifdef USE_I2S
		     , FALSE, pI2CMaster, DAC_I2C_ADDRESS
#endif
#endif
	),
	m_pMIDIDevice (0),
	m_pKeyboard (0),
#if RASPPI <= 3 && defined (USE_USB_FIQ)
	m_Serial (pInterrupt, FALSE),
#else
	m_Serial (pInterrupt, TRUE),
#endif
	m_bUseSerial (FALSE),
	m_nSerialState (0),
	m_nSampleCount (0),
	m_nFrequency (0),
	m_nPrevFrequency (0),
	m_ucKeyNumber (KEY_NONE),
	m_bSetVolume (FALSE)
{
	s_pThis = this;

	m_nLowLevel     = GetRangeMin () * VOLUME_PERCENT / 100;
	m_nHighLevel    = GetRangeMax () * VOLUME_PERCENT / 100;
	m_nNullLevel    = (m_nHighLevel + m_nLowLevel) / 2;
	m_nCurrentLevel = m_nNullLevel;
}

CMiniOrgan::~CMiniOrgan (void)
{
	s_pThis = 0;
}

boolean CMiniOrgan::Initialize (void)
{
	CLogger::Get ()->Write (FromMiniOrgan, LogNotice,
				"Please attach an USB keyboard or use serial MIDI!");

	if (m_Serial.Initialize (31250))
	{
		m_bUseSerial = TRUE;

		return TRUE;
	}

	return FALSE;
}

void CMiniOrgan::Process (boolean bPlugAndPlayUpdated)
{
	// The sound controller is callable from TASK_LEVEL only. That's why we must do
	// this here and not in MIDIPacketHandler(), which is called from IRQ_LEVEL too.
	if (m_bSetVolume)
	{
		m_bSetVolume = FALSE;

		// The sound controller is optional, so we check, if it exists.
		CSoundController *pController = GetController ();
		if (pController)
		{
			CSoundController::TControlInfo Info = pController->GetControlInfo (
				CSoundController::ControlVolume, CSoundController::JackDefaultOut,
				CSoundController::ChannelAll);
			if (Info.Supported)
			{
				int nVolume = m_uchVolume;
				nVolume *= Info.RangeMax - Info.RangeMin;
				nVolume /= 127;
				nVolume += Info.RangeMin;

				pController->SetControl (CSoundController::ControlVolume,
							 CSoundController::JackDefaultOut,
							 CSoundController::ChannelAll, nVolume);
			}
		}
	}

	if (m_pMIDIDevice != 0)
	{
		return;
	}

	if (bPlugAndPlayUpdated)
	{
		m_pMIDIDevice =
			(CUSBMIDIDevice *) CDeviceNameService::Get ()->GetDevice ("umidi1", FALSE);
		if (m_pMIDIDevice != 0)
		{
			m_pMIDIDevice->RegisterRemovedHandler (USBDeviceRemovedHandler);
			m_pMIDIDevice->RegisterPacketHandler (MIDIPacketHandler);

			return;
		}
	}

	if (m_pKeyboard != 0)
	{
		return;
	}

	if (bPlugAndPlayUpdated)
	{
		m_pKeyboard =
			(CUSBKeyboardDevice *) CDeviceNameService::Get ()->GetDevice ("ukbd1", FALSE);
		if (m_pKeyboard != 0)
		{
			m_pKeyboard->RegisterRemovedHandler (USBDeviceRemovedHandler);
			m_pKeyboard->RegisterKeyStatusHandlerRaw (KeyStatusHandlerRaw);

			return;
		}
	}

	if (!m_bUseSerial)
	{
		return;
	}

	// Read serial MIDI data
	u8 Buffer[20];
	int nResult = m_Serial.Read (Buffer, sizeof Buffer);
	if (nResult <= 0)
	{
		return;
	}

	// Process MIDI messages
	// See: https://www.midi.org/specifications/item/table-1-summary-of-midi-message
	for (int i = 0; i < nResult; i++)
	{
		u8 uchData = Buffer[i];

		switch (m_nSerialState)
		{
		case 0:
		MIDIRestart:
			if (   (uchData & 0xE0) == 0x80		// Note on or off, all channels
			    || (uchData & 0xF0) == 0xB0)	// MIDI CC, all channels
			{
				m_SerialMessage[m_nSerialState++] = uchData;
			}
			break;

		case 1:
		case 2:
			if (uchData & 0x80)			// got status when parameter expected
			{
				m_nSerialState = 0;

				goto MIDIRestart;
			}

			m_SerialMessage[m_nSerialState++] = uchData;

			if (m_nSerialState == 3)		// message is complete
			{
				MIDIPacketHandler (0, m_SerialMessage, sizeof m_SerialMessage);

				m_nSerialState = 0;
			}
			break;

		default:
			assert (0);
			break;
		}
	}
}

#ifdef USE_USB

unsigned CMiniOrgan::GetChunk (s16 *pBuffer, unsigned nChunkSize)
{
	unsigned nChannels = GetHWTXChannels ();
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
		nSampleDelay = (SAMPLE_RATE/2 + m_nFrequency/2) / m_nFrequency;
	}

	for (; nChunkSize > 0; nChunkSize -= nChannels)		// fill the whole buffer
	{
		s16 nSample = (s16) m_nNullLevel;

		if (m_nFrequency != 0)			// key pressed?
		{
			// change output level if required to generate a square wave
			if (++m_nSampleCount >= nSampleDelay)
			{
				m_nSampleCount = 0;

				if (m_nCurrentLevel < m_nHighLevel)
				{
					m_nCurrentLevel = m_nHighLevel;
				}
				else
				{
					m_nCurrentLevel = m_nLowLevel;
				}
			}

			nSample = (s16) m_nCurrentLevel;
		}

		for (unsigned i = 0; i < nChannels; i++)
		{
			*pBuffer++ = nSample;
		}
	}

	return nResult;
}

#endif

unsigned CMiniOrgan::GetChunk (u32 *pBuffer, unsigned nChunkSize)
{
	unsigned nChannels = GetHWTXChannels ();
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
		nSampleDelay = (SAMPLE_RATE/2 + m_nFrequency/2) / m_nFrequency;
	}

#ifdef USE_HDMI
	unsigned nFrame = 0;
#endif
	for (; nChunkSize > 0; nChunkSize -= nChannels)		// fill the whole buffer
	{
		u32 nSample = (u32) m_nNullLevel;

		if (m_nFrequency != 0)			// key pressed?
		{
			// change output level if required to generate a square wave
			if (++m_nSampleCount >= nSampleDelay)
			{
				m_nSampleCount = 0;

				if (m_nCurrentLevel < m_nHighLevel)
				{
					m_nCurrentLevel = m_nHighLevel;
				}
				else
				{
					m_nCurrentLevel = m_nLowLevel;
				}
			}

			nSample = (u32) m_nCurrentLevel;
		}

#ifdef USE_HDMI
		nSample = ConvertIEC958Sample (nSample, nFrame);

		if (++nFrame == IEC958_FRAMES_PER_BLOCK)
		{
			nFrame = 0;
		}
#endif

		for (unsigned i = 0; i < nChannels; i++)
		{
#ifdef USE_USB
			*pBuffer = nSample;
			pBuffer = (u32 *) ((u8 *) pBuffer + 3);
#else
			*pBuffer++ = nSample;
#endif
		}
	}

	return nResult;
}

void CMiniOrgan::MIDIPacketHandler (unsigned nCable, u8 *pPacket, unsigned nLength)
{
	assert (s_pThis != 0);

	// The packet contents are just normal MIDI data - see
	// https://www.midi.org/specifications/item/table-1-summary-of-midi-message

	if (nLength < 3)
	{
		return;
	}

	u8 ucStatus    = pPacket[0];
	//u8 ucChannel   = ucStatus & 0x0F;
	u8 ucType      = ucStatus >> 4;
	u8 ucKeyNumber = pPacket[1];
	u8 ucVelocity  = pPacket[2];

	if (ucType == MIDI_NOTE_ON)
	{
		if (   ucVelocity > 0
		    && ucKeyNumber < sizeof s_KeyFrequency / sizeof s_KeyFrequency[0])
		{
			s_pThis->m_ucKeyNumber = ucKeyNumber;
			s_pThis->m_nFrequency = (unsigned) (s_KeyFrequency[ucKeyNumber] + 0.5);
		}
		else
		{
			if (s_pThis->m_ucKeyNumber == ucKeyNumber)
			{
				s_pThis->m_ucKeyNumber = KEY_NONE;
				s_pThis->m_nFrequency = 0;
			}
		}
	}
	else if (ucType == MIDI_NOTE_OFF)
	{
		if (s_pThis->m_ucKeyNumber == ucKeyNumber)
		{
			s_pThis->m_ucKeyNumber = KEY_NONE;
			s_pThis->m_nFrequency = 0;
		}
	}
	else if (ucType == MIDI_CC)
	{
		if (pPacket[1] == MIDI_CC_VOLUME)
		{
			s_pThis->m_uchVolume = pPacket[2];
			s_pThis->m_bSetVolume = TRUE;
		}
	}
}

void CMiniOrgan::KeyStatusHandlerRaw (unsigned char ucModifiers, const unsigned char RawKeys[6])
{
	assert (s_pThis != 0);

	// find the key code of a pressed key
	char chKey = '\0';
	for (unsigned i = 0; i <= 5; i++)
	{
		u8 ucKeyCode = RawKeys[i];
		if (ucKeyCode != 0)
		{
			if (0x04 <= ucKeyCode && ucKeyCode <= 0x1D)
			{
				chKey = RawKeys[i]-0x04+'A';	// key code of 'A' is 0x04
			}
			else if (ucKeyCode == 0x36)
			{
				chKey = ',';			// key code of ',' is 0x36
			}

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
				u8 ucKeyNumber = s_Keys[i].KeyNumber;

				assert (ucKeyNumber < sizeof s_KeyFrequency / sizeof s_KeyFrequency[0]);
				s_pThis->m_nFrequency = (unsigned) (s_KeyFrequency[ucKeyNumber] + 0.5);

				return;
			}
		}
	}

	s_pThis->m_nFrequency = 0;
}

void CMiniOrgan::USBDeviceRemovedHandler (CDevice *pDevice, void *pContext)
{
	assert (s_pThis != 0);

	if (s_pThis->m_pMIDIDevice == (CUSBMIDIDevice *) pDevice)
	{
		CLogger::Get ()->Write (FromMiniOrgan, LogDebug, "USB MIDI keyboard removed");

		s_pThis->m_pMIDIDevice = 0;
	}
	else if (s_pThis->m_pKeyboard == (CUSBKeyboardDevice *) pDevice)
	{
		CLogger::Get ()->Write (FromMiniOrgan, LogDebug, "USB PC keyboard removed");

		s_pThis->m_pKeyboard = 0;
	}
}
