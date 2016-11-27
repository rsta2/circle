//
// usbgamepad.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2016  R. Stange <rsta2@o2online.de>
//
// Ported from the USPi driver which is:
// 	Copyright (C) 2014  M. Maccaferri <macca@maccasoft.com>
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
#ifndef _circle_usb_usbgamepad_h
#define _circle_usb_usbgamepad_h

#include <circle/usb/usbhiddevice.h>
#include <circle/types.h>

#define MAX_AXIS    6
#define MAX_HATS    6

struct TGamePadState
{
	int naxes;
	struct
	{
		int value;
		int minimum;
		int maximum;
	}
	axes[MAX_AXIS];

	int nhats;
	int hats[MAX_HATS];

	int nbuttons;
	unsigned buttons;
};

typedef void TGamePadStatusHandler (unsigned nDeviceIndex, const TGamePadState *pGamePadState);

class CUSBGamePadDevice : public CUSBHIDDevice
{
public:
	CUSBGamePadDevice (CUSBFunction *pFunction);
	~CUSBGamePadDevice (void);

	boolean Configure (void);

	const TGamePadState *GetReport (void);		// returns 0 on failure

	void RegisterStatusHandler (TGamePadStatusHandler *pStatusHandler);

private:
	void ReportHandler (const u8 *pReport);

	void DecodeReport (const u8 *pReportBuffer);
	static u32 BitGetUnsigned (const void *buffer, u32 offset, u32 length);
	static s32 BitGetSigned (const void *buffer, u32 offset, u32 length);

	void PS3Configure (void);

private:
	TGamePadState m_State;
	TGamePadStatusHandler *m_pStatusHandler;

	u8 *m_pHIDReportDescriptor;
	u16 m_usReportDescriptorLength;
	u16 m_usReportSize;

	unsigned m_nDeviceNumber;
	static unsigned s_nDeviceNumber;
};

#endif
