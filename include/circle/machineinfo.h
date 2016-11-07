//
// machineinfo.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016  R. Stange <rsta2@o2online.de>
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
	MachineModel2B,
	MachineModel3B,
	MachineModelCM,
	MachineModelUnknown
};

enum TSoCType
{
	SoCTypeBCM2835,
	SoCTypeBCM2836,
	SoCTypeBCM2837,
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
	CMachineInfo (void);
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
	unsigned GetClockRate (u32 nClockId) const;	// see circle/bcmpropertytags.h for nClockId
	unsigned GetGPIOPin (TGPIOVirtualPin Pin) const;// see circle/gpiopin.h for Pin
	unsigned GetDevice (TDeviceId DeviceId) const;

	static CMachineInfo *Get (void);

private:
	u32		m_nRevisionRaw;
	TMachineModel	m_MachineModel;
	unsigned	m_nModelMajor;
	unsigned	m_nModelRevision;
	TSoCType	m_SoCType;
	unsigned	m_nRAMSize;

	static CMachineInfo *s_pThis;
};

#endif
