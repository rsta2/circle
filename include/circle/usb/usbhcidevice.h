//
/// \file usbhcidevice.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2019-2023  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_usbhcidevice_h
#define _circle_usb_usbhcidevice_h

/// \class CUSBHCIDevice
/// \brief Alias for CDWHCIDevice, CXHCIDevice or CUSBSubSystem, depending on Raspberry Pi model

#if RASPPI <= 3
	#include <circle/usb/dwhcidevice.h>
	#define CUSBHCIDevice	CDWHCIDevice
#elif RASPPI == 4
	#include <circle/usb/xhcidevice.h>
	#define CUSBHCIDevice	CXHCIDevice
#else
	#include <circle/usb/usbsubsystem.h>
	#define CUSBHCIDevice	CUSBSubSystem
#endif

#endif
