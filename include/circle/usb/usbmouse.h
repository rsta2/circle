//
// usbmouse.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_usbmouse_h
#define _circle_usb_usbmouse_h

#include <circle/usb/usbhiddevice.h>
#include <circle/types.h>

#define MOUSE_DISPLACEMENT_MIN	-127
#define MOUSE_DISPLACEMENT_MAX	127

#define MOUSE_BUTTON_LEFT	(1 << 0)
#define MOUSE_BUTTON_RIGHT	(1 << 1)
#define MOUSE_BUTTON_MIDDLE	(1 << 2)

typedef void TMouseStatusHandler (unsigned nButtons, int nDisplacementX, int nDisplacementY);

class CUSBMouseDevice : public CUSBHIDDevice
{
public:
	CUSBMouseDevice (CUSBFunction *pFunction);
	~CUSBMouseDevice (void);

	boolean Configure (void);

	void RegisterStatusHandler (TMouseStatusHandler *pStatusHandler);

private:
	void ReportHandler (const u8 *pReport);

private:
	TMouseStatusHandler *m_pStatusHandler;

	static unsigned s_nDeviceNumber;
};

#endif
