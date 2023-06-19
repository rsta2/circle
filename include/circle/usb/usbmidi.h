//
/// \file usbmidi.h
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
//
#ifndef _circle_usb_usbmidi_h
#define _circle_usb_usbmidi_h

#include <circle/device.h>
#include <circle/numberpool.h>
#include <circle/types.h>

/// \param nCable  Cable number (0-15)
/// \param pPacket Pointer to one received MIDI packet
/// \param nLength Number of valid bytes in packet (1-3)
typedef void TMIDIPacketHandler (unsigned nCable, u8 *pPacket, unsigned nLength);
/// \param nCable  Cable number (0-15)
/// \param pPacket Pointer to one received MIDI packet
/// \param nLength Number of valid bytes in packet (1-3)
/// \param nDevice Device number of the MIDI device (e.g. 1 for "umidi1")
/// \param pParam User parameter
typedef void TMIDIPacketHandlerEx (unsigned nCable, u8 *pPacket, unsigned nLength,
				   unsigned nDevice, void *pParam);

class CUSBMIDIDevice : public CDevice	/// Interface device for USB Audio Class MIDI 1.0 devices
{
public:
	static const size_t EventPacketSize = 4;

public:
	CUSBMIDIDevice (void);
	~CUSBMIDIDevice (void);

	/// \brief Register a handler, which is called, when a MIDI packet arrives
	/// \param pPacketHandler Pointer to the handler
	void RegisterPacketHandler (TMIDIPacketHandler *pPacketHandler);
	/// \brief Register a handler, which is called, when a MIDI packet arrives
	/// \param pPacketHandler Pointer to the handler
	/// \param pParam User parameter, handed over to the handler
	void RegisterPacketHandler (TMIDIPacketHandlerEx *pPacketHandler, void *pParam);

	/// \brief Send one or more packets in encoded USB MIDI event packet format
	/// \param pData Pointer to the packet buffer
	/// \param nLength Length of packet buffer in bytes (multiple of 4)
	/// \return Operation successful?
	/// \note Fails, if nLength is not multiple of 4 or send is not supported\n
	///	  Format is not validated
	boolean SendEventPackets (const u8 *pData, unsigned nLength);

	/// \brief Send one or more messages in plain MIDI message format
	/// \param nCable Cable number (0-15)
	/// \param pData Pointer to the message buffer
	/// \param nLength Length of message buffer in bytes
	/// \return Operation successful?
	/// \note Fails, if format is invalid or send is not supported
	boolean SendPlainMIDI (unsigned nCable, const u8 *pData, unsigned nLength);

	/// \brief Generate MIDI CC "All Sound Off" event (120), when an USB error occurs?
	/// \param bEnable Set to TRUE to enable function
	/// \note This will generate an event for each MIDI channel (1-16) for MIDI cable 0.
	void SetAllSoundOffOnUSBError (boolean bEnable);

private:
	// returns TRUE, if valid MIDI data has been received
	boolean CallPacketHandler (u8 *pData, unsigned nLength);

	typedef boolean TSendEventsHandler (const u8 *pData, unsigned nLength, void *pParam);
	void RegisterSendEventsHandler (TSendEventsHandler *pHandler, void *pParam);

	boolean GetAllSoundOffOnUSBError (void) const;

	friend class CUSBMIDIHostDevice;
	friend class CUSBMIDIGadgetEndpoint;

private:
	TMIDIPacketHandlerEx *m_pPacketHandler;
	void *m_pPacketHandlerParam;

	TSendEventsHandler *m_pSendEventsHandler;
	void *m_pSendEventsParam;

	boolean m_bAllSoundOff;

	unsigned m_nDeviceNumber;
	static CNumberPool s_DeviceNumberPool;
};

#endif
