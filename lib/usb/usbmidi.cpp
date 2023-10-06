//
// usbmidi.c
//
// Ported from the USPi driver which is:
// 	Copyright (C) 2016  J. Otto <joshua.t.otto@gmail.com>
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

// Refer to "Universal Serial Bus Device Class Specification for MIDI Devices"

#include <circle/usb/usbmidi.h>
#include <circle/devicenameservice.h>
#include <circle/logger.h>
#include <circle/debug.h>
#include <circle/util.h>
#include <assert.h>

static const char FromMIDI[] = "umidi";
static const char DevicePrefix[] = "umidi";

CNumberPool CUSBMIDIDevice::s_DeviceNumberPool (1);

// This handy table from the Linux driver encodes the mapping between MIDI
// packet Code Index Number values and encapsulated packet lengths.
static const unsigned cin_to_length[] = {
	0, 0, 2, 3, 3, 1, 2, 3, 3, 3, 3, 3, 2, 2, 3, 1
};

CUSBMIDIDevice::CUSBMIDIDevice (void)
:	m_pPacketHandler (0),
	m_pSendEventsHandler (0),
	m_bAllSoundOff (FALSE),
	m_nDeviceNumber (s_DeviceNumberPool.AllocateNumber (TRUE, FromMIDI))
{
	CDeviceNameService::Get ()->AddDevice (DevicePrefix, m_nDeviceNumber, this, FALSE);
}

CUSBMIDIDevice::~CUSBMIDIDevice (void)
{
	CDeviceNameService::Get ()->RemoveDevice (DevicePrefix, m_nDeviceNumber, FALSE);

	s_DeviceNumberPool.FreeNumber (m_nDeviceNumber);
}

static void ProxyHandler (unsigned nCable, u8 *pPacket, unsigned nLength,
			  unsigned nDevice, void *pParam)
{
	assert (pParam);
	((TMIDIPacketHandler *) pParam) (nCable, pPacket, nLength);
}

void CUSBMIDIDevice::RegisterPacketHandler (TMIDIPacketHandler *pPacketHandler)
{
	assert (pPacketHandler);
	RegisterPacketHandler (ProxyHandler, (void *) pPacketHandler);
}

void CUSBMIDIDevice::RegisterPacketHandler (TMIDIPacketHandlerEx *pPacketHandler, void *pParam)
{
	m_pPacketHandlerParam = pParam;

	assert (m_pPacketHandler == 0);
	m_pPacketHandler = pPacketHandler;
	assert (m_pPacketHandler != 0);
}

boolean CUSBMIDIDevice::SendEventPackets (const u8 *pData, unsigned nLength)
{
	if (!m_pSendEventsHandler)
	{
		return FALSE;
	}

	return (*m_pSendEventsHandler) (pData, nLength, m_pSendEventsParam);
}

boolean CUSBMIDIDevice::SendPlainMIDI (unsigned nCable, const u8 *pData, unsigned nLength)
{
	assert (nCable <= 15);
	assert (pData != 0);
	assert (nLength > 0);

	size_t nBufferSize = nLength * 4;		// worst case
	size_t nBufferValid = 0;
	u8 Buffer[nBufferSize];
	u8 *pBuffer = Buffer;

	unsigned nState = 0;
	unsigned nPacketLength = 0;
	u8 *pSysExStart = 0;
	while (nLength--)
	{
		u8 uchByte = *pData++;
		switch (nState)
		{
		case 0:					// handle MIDI status code
			if (uchByte < 0xF0)		// channel voice message
			{
				*pBuffer++ = (nCable << 4) | (uchByte >> 4);
				nBufferValid++;
				nPacketLength = cin_to_length[uchByte >> 4];
				nState = 1;
			}
			else				// system common messages
			{
				switch (uchByte)
				{
				case 0xF0:		// sysex
					pSysExStart = pBuffer;
					*pBuffer++ = (nCable << 4) | 4;
					nBufferValid++;
					nPacketLength = 0;
					nState = 2;
					goto StateSysEx;

				case 0xF1:		// time code
				case 0xF3:		// song select
					*pBuffer++ = (nCable << 4) | 2;
					nBufferValid++;
					nPacketLength = 2;
					nState = 1;
					break;

				case 0xF2:		// song position pointer
					*pBuffer++ = (nCable << 4) | 3;
					nBufferValid++;
					nPacketLength = 3;
					nState = 1;
					break;

				case 0xF6:		// tune request
				case 0xF8:		// timing clock
				case 0xFA:		// start
				case 0xFB:		// continue
				case 0xFC:		// stop
				case 0xFE:		// active sensing
				case 0xFF:		// reset
					*pBuffer++ = (nCable << 4) | 5;
					nBufferValid++;
					nPacketLength = 1;
					nState = 1;
					break;

				default:
					CLogger::Get ()->Write (FromMIDI, LogWarning,
								"Unsupported MIDI status code (0x%X)",
								(unsigned) uchByte);
					return FALSE;
				}
			}
			// fall through

		case 1:					// copy MIDI data bytes
			*pBuffer++ = uchByte;
			nBufferValid++;
			if (--nPacketLength == 0)
			{
				while ((nBufferValid & 3) != 0)		// padding
				{
					*pBuffer++ = 0;
					nBufferValid++;
				}

				nState = 0;
			}
			break;

		StateSysEx:				// handle sysex message
		case 2:
			if (nPacketLength == 3)		// current event packet full?
			{
				pSysExStart = pBuffer;	// start next packet
				*pBuffer++ = (nCable << 4) | 4;
				nBufferValid++;
				nPacketLength = 0;
			}

			*pBuffer++ = uchByte;		// copy sysex data bytes
			nBufferValid++;
			nPacketLength++;

			if (uchByte == 0xF7)		// end of sysex message?
			{
				*pSysExStart = (nCable << 4) | (nPacketLength + 4);	// set CIN

				while (nPacketLength++ < 3)	// padding
				{
					*pBuffer++ = 0;
					nBufferValid++;
				}

				nState = 0;
			}
			break;

		default:
			assert (0);
			break;
		}
	}

	if (nState != 0)
	{
		CLogger::Get ()->Write (FromMIDI, LogWarning, "Incomplete MIDI message");

		return FALSE;
	}

	//debug_hexdump (Buffer, nBufferValid, FromMIDI);

	return SendEventPackets (Buffer, nBufferValid);
}

void CUSBMIDIDevice::SetAllSoundOffOnUSBError (boolean bEnable)
{
	m_bAllSoundOff = bEnable;
}

boolean CUSBMIDIDevice::CallPacketHandler (u8 *pData, unsigned nLength)
{
	assert (pData);
	assert (nLength % EventPacketSize == 0);

	boolean bResult = FALSE;

	u8 *pEnd = pData + nLength;
	for (u8 *pPacket = pData; pPacket < pEnd; pPacket += EventPacketSize)
	{
		// Follow the Linux driver's example and ignore packets with Cable
		// Number == Code Index Number == 0, which some devices seem to
		// generate as padding in spite of their status as reserved.
		if (pPacket[0] != 0)
		{
			if (m_pPacketHandler != 0)
			{
				unsigned nCable = pPacket[0] >> 4;
				unsigned nLength = cin_to_length[pPacket[0] & 0x0F];
				(*m_pPacketHandler) (nCable, pPacket+1, nLength,
						     m_nDeviceNumber, m_pPacketHandlerParam);
			}

			bResult = TRUE;
		}
	}

	return bResult;
}

void CUSBMIDIDevice::RegisterSendEventsHandler (TSendEventsHandler *pHandler, void *pParam)
{
	m_pSendEventsParam = pParam;

	assert (!m_pSendEventsHandler);
	m_pSendEventsHandler = pHandler;
	assert (m_pSendEventsHandler);
}

boolean CUSBMIDIDevice::GetAllSoundOffOnUSBError (void) const
{
	return m_bAllSoundOff;
}
