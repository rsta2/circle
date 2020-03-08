//
// machineinfo.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2020  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_machineinfo_h
#define _circle_machineinfo_h

#include <circle/bcmpropertytags.h>
#include <circle/gpiopin.h>
#include <circle/macros.h>
#include <circle/types.h>

enum TMachineModel
{
	MachineModelA,
	MachineModelBRelease1MB256,
	MachineModelBRelease2MB256,
	MachineModelBRelease2MB512,
	MachineModelAPlus,
	MachineModelBPlus,
	MachineModelZero,
	MachineModelZeroW,
	MachineModel2B,
	MachineModel3B,
	MachineModel3APlus,
	MachineModel3BPlus,
	MachineModelCM,
	MachineModelCM3,
	MachineModelCM3Plus,
	MachineModel4B,
	MachineModelUnknown
};

enum TSoCType
{
	SoCTypeBCM2835,
	SoCTypeBCM2836,
	SoCTypeBCM2837,
	SoCTypeBCM2711,
	SoCTypeUnknown
};

enum TDeviceId
{
	DeviceI2CMaster,
	DeviceUnkown
};

class CMachineInfo
{
public:
	CMachineInfo (void) NOOPT;
	~CMachineInfo (void);

	// Basic info
	TMachineModel GetMachineModel (void) const;
	const char *GetMachineName (void) const;

	// Detailed info
	unsigned GetModelMajor (void) const;		// 1..3, 0 on error
	unsigned GetModelRevision (void) const;		// 1-based, 0 on error
	TSoCType GetSoCType (void) const;
	unsigned GetRAMSize (void) const;		// MByte, 0 on error

	const char *GetSoCName (void) const;

	// Raw info
	u32 GetRevisionRaw (void) const;

	// Clock and peripheral info
	unsigned GetActLEDInfo (void) const;
#define ACTLED_PIN_MASK		0x3F
#define ACTLED_ACTIVE_LOW	0x40
#define ACTLED_VIRTUAL_PIN	0x80
#define ACTLED_UNKNOWN		(ACTLED_VIRTUAL_PIN | 0)
	unsigned GetClockRate (u32 nClockId) const;	// see circle/bcmpropertytags.h for nClockId
	unsigned GetGPIOPin (TGPIOVirtualPin Pin) const;// see circle/gpiopin.h for Pin
	unsigned GetGPIOClockSourceRate (unsigned nSourceId);
#define GPIO_CLOCK_SOURCE_ID_MAX	15		// source ID is 0-15
#define GPIO_CLOCK_SOURCE_UNUSED	0		// returned for unused clock sources
	unsigned GetDevice (TDeviceId DeviceId) const;

	// returns TRUE, if the left PWM audio channel is PWM1 (not PWM0)
	boolean ArePWMChannelsSwapped (void) const;

	// DMA channel resource management
#if RASPPI <= 3
#define DMA_CHANNEL_MAX		12			// channels 0-12 are supported
#else
#define DMA_CHANNEL_MAX		7			// TODO: channels 0-7 are supported
#endif
#define DMA_CHANNEL__MASK	0x0F			// explicit channel number
#define DMA_CHANNEL_NONE	0x80			// returned if no channel available
#define DMA_CHANNEL_NORMAL	0x81			// normal DMA engine requested
#define DMA_CHANNEL_LITE	0x82			// lite (or normal) DMA engine requested
	// nChannel must be DMA_CHANNEL_NORMAL, DMA_CHANNEL_LITE or an explicit channel number
	// returns the allocated channel number or DMA_CHANNEL_NONE on failure
	unsigned AllocateDMAChannel (unsigned nChannel);
	void FreeDMAChannel (unsigned nChannel);

	static CMachineInfo *Get (void);

private:
	u32		m_nRevisionRaw;
	TMachineModel	m_MachineModel;
	unsigned	m_nModelMajor;
	unsigned	m_nModelRevision;
	TSoCType	m_SoCType;
	unsigned	m_nRAMSize;

	u16		m_usDMAChannelMap;		// channel bit set if channel is free

	static CMachineInfo *s_pThis;
};

#endif
