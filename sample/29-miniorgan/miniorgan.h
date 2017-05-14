//
// miniorgan.h
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
#ifndef _miniorgan_h
#define _miniorgan_h

#include <circle/pwmsoundbasedevice.h>
#include <circle/interrupt.h>
#include <circle/serial.h>
#include <circle/types.h>

struct TNoteInfo
{
	char	Key;
	u8	KeyNumber;	// MIDI number
};

class CMiniOrgan : public CPWMSoundBaseDevice
{
public:
	CMiniOrgan (CInterruptSystem *pInterrupt);
	~CMiniOrgan (void);

	boolean Initialize (void);

	void Process (void);

	unsigned GetChunk (u32 *pBuffer, unsigned nChunkSize);

private:
	static void MIDIPacketHandler (unsigned nCable, u8 *pPacket, unsigned nLength);

	static void KeyStatusHandlerRaw (unsigned char ucModifiers, const unsigned char RawKeys[6]);

private:
	CSerialDevice m_Serial;
	boolean m_bUseSerial;
	unsigned m_nSerialState;
	u8 m_SerialMessage[3];

	unsigned m_nHighLevel;
	unsigned m_nCurrentLevel;
	unsigned m_nSampleCount;
	unsigned m_nFrequency;		// 0 if no key pressed
	unsigned m_nPrevFrequency;

	u8 m_ucKeyNumber;

	static const float s_KeyFrequency[];
	static const TNoteInfo s_Keys[];

	static CMiniOrgan *s_pThis;
};

#endif
