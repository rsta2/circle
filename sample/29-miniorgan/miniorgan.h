//
// miniorgan.h
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
#ifndef _miniorgan_h
#define _miniorgan_h

// define only one
//#define USE_I2S
//#define USE_HDMI
//#define USE_USB

#ifdef USE_I2S
	#include <circle/sound/i2ssoundbasedevice.h>
	#define SOUND_CLASS	CI2SSoundBaseDevice
	#define SAMPLE_RATE	192000
	#define CHUNK_SIZE	8192
	#define DAC_I2C_ADDRESS	0		// I2C slave address of the DAC (0 for auto probing)
#elif defined (USE_HDMI)
	#include <circle/sound/hdmisoundbasedevice.h>
	#define SOUND_CLASS	CHDMISoundBaseDevice
	#define SAMPLE_RATE	48000
	#define CHUNK_SIZE	(384 * 10)
#elif defined (USE_USB)
	#include <circle/sound/usbsoundbasedevice.h>
	#define SOUND_CLASS	CUSBSoundBaseDevice
	#define SAMPLE_RATE	48000
#else
	#include <circle/sound/pwmsoundbasedevice.h>
	#define SOUND_CLASS	CPWMSoundBaseDevice
	#define SAMPLE_RATE	48000
	#define CHUNK_SIZE	2048
#endif

#include <circle/interrupt.h>
#include <circle/i2cmaster.h>
#include <circle/usb/usbmidi.h>
#include <circle/usb/usbkeyboard.h>
#include <circle/serial.h>
#include <circle/types.h>

struct TNoteInfo
{
	char	Key;
	u8	KeyNumber;	// MIDI number
};

class CMiniOrgan : public SOUND_CLASS
{
public:
	CMiniOrgan (CInterruptSystem *pInterrupt, CI2CMaster *pI2CMaster);
	~CMiniOrgan (void);

	boolean Initialize (void);

	void Process (boolean bPlugAndPlayUpdated);

#ifdef USE_USB
	unsigned GetChunk (s16 *pBuffer, unsigned nChunkSize);
#endif
	unsigned GetChunk (u32 *pBuffer, unsigned nChunkSize);

private:
	static void MIDIPacketHandler (unsigned nCable, u8 *pPacket, unsigned nLength);

	static void KeyStatusHandlerRaw (unsigned char ucModifiers, const unsigned char RawKeys[6]);

	static void USBDeviceRemovedHandler (CDevice *pDevice, void *pContext);

private:
	CUSBMIDIDevice     * volatile m_pMIDIDevice;
	CUSBKeyboardDevice * volatile m_pKeyboard;

	CSerialDevice m_Serial;
	boolean m_bUseSerial;
	unsigned m_nSerialState;
	u8 m_SerialMessage[3];

	int      m_nLowLevel;
	int      m_nNullLevel;
	int      m_nHighLevel;
	int      m_nCurrentLevel;
	unsigned m_nSampleCount;
	unsigned m_nFrequency;		// 0 if no key pressed
	unsigned m_nPrevFrequency;

	u8 m_ucKeyNumber;

	boolean m_bSetVolume;
	u8 m_uchVolume;

	static const float s_KeyFrequency[];
	static const TNoteInfo s_Keys[];

	static CMiniOrgan *s_pThis;
};

#endif
