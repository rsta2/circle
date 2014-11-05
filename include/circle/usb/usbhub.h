//
// usbhub.h
//
// Definitions for USB hubs
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
#ifndef _usbhub_h
#define _usbhub_h

#include <circle/macros.h>

// Configuration
#define USB_HUB_MAX_PORTS		8		// TODO

// Device Class
#define USB_DEVICE_CLASS_HUB		9

// Class-specific Requests
#define RESET_TT			9

// Descriptor Type
#define DESCRIPTOR_HUB			0x29

// Feature Selectors
#define PORT_RESET			4
#define PORT_POWER			8

// Hub Descriptor
struct TUSBHubDescriptor
{
	unsigned char	bDescLength;
	unsigned char	bDescriptorType;
	unsigned char	bNbrPorts;
	unsigned short	wHubCharacteristics;
		#define HUB_POWER_MODE(reg)			((reg) & 3)
			#define HUB_POWER_MODE_GANGED		0
			#define HUB_POWER_MODE_INDIVIDUAL	1
		#define HUB_TT_THINK_TIME(reg)			(((reg) >> 5) & 3)
	unsigned char	bPwrOn2PwrGood;
	unsigned char	bHubContrCurrent;
	unsigned char	DeviceRemoveable[1];		// max. 8 ports
	unsigned char	PortPwrCtrlMask[1];		// max. 8 ports
}
PACKED;

struct TUSBHubStatus
{
	unsigned short	wHubStatus;
		#define HUB_LOCAL_POWER_LOST__MASK	(1 << 0)
		#define HUB_OVER_CURRENT__MASK		(1 << 1)
	unsigned short	wHubChange;
		#define C_HUB_LOCAL_POWER_LOST__MASK	(1 << 0)
		#define C_HUB_OVER_CURRENT__MASK	(1 << 1)
}
PACKED;

struct TUSBPortStatus
{
	unsigned short	wPortStatus;
		#define PORT_CONNECTION__MASK		(1 << 0)
		#define PORT_ENABLE__MASK		(1 << 1)
		#define PORT_OVER_CURRENT__MASK		(1 << 3)
		#define PORT_RESET__MASK		(1 << 4)
		#define PORT_POWER__MASK		(1 << 8)
		#define PORT_LOW_SPEED__MASK		(1 << 9)
		#define PORT_HIGH_SPEED__MASK		(1 << 10)
	unsigned short	wChangeStatus;
}
PACKED;

#endif
