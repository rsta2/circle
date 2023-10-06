//
// usbcontroller.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2023  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_usbcontroller_h
#define _circle_usb_usbcontroller_h

#include <circle/types.h>

class CUSBController	/// Generic USB (host or gadget) controller
{
public:
	/// \brief Initialize controller
	/// \param bScanDevices Immediately scan for connected devices?
	/// \return Operation successful?
	virtual boolean Initialize (boolean bScanDevices = TRUE) = 0;

	/// \brief Update info about connected devices
	/// \return TRUE if information has been modified
	/// \note First call always returns TRUE.
	virtual boolean UpdatePlugAndPlay (void) = 0;
};

#endif
