//
// devicenameservice.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2018  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_devicenameservice_h
#define _circle_devicenameservice_h

#include <circle/device.h>
#include <circle/spinlock.h>
#include <circle/types.h>

struct TDeviceInfo
{
	TDeviceInfo	*pNext;
	char		*pName;
	CDevice		*pDevice;
	boolean		 bBlockDevice;
};

class CDeviceNameService  /// Devices can be registered by name and retrieved later by this name
{
public:
	CDeviceNameService (void);
	~CDeviceNameService (void);

	/// \param pName	Device name string
	/// \param pDevice	Pointer to the device object
	/// \param bBlockDevice TRUE if this is a block device, otherwise character device
	void AddDevice (const char *pName, CDevice *pDevice, boolean bBlockDevice);
	/// \param pPrefix	Device name prefix string
	/// \param nIndex	Device name index
	/// \param pDevice	Pointer to the device object
	/// \param bBlockDevice TRUE if this is a block device, otherwise character device
	void AddDevice (const char *pPrefix, unsigned nIndex, CDevice *pDevice, boolean bBlockDevice);

	/// \param pName	Device name string
	/// \param bBlockDevice TRUE if this is a block device, otherwise character device
	void RemoveDevice (const char *pName, boolean bBlockDevice);
	/// \param pPrefix	Device name prefix string
	/// \param nIndex	Device name index
	/// \param bBlockDevice TRUE if this is a block device, otherwise character device
	void RemoveDevice (const char *pPrefix, unsigned nIndex, boolean bBlockDevice);

	/// \param pName	Device name string
	/// \param bBlockDevice TRUE if this is a block device, otherwise character device
	/// \return Pointer to the device object or 0 if not found
	CDevice *GetDevice (const char *pName, boolean bBlockDevice);
	/// \param pPrefix	Device name prefix string
	/// \param nIndex	Device name index
	/// \param bBlockDevice TRUE if this is a block device, otherwise character device
	/// \return Pointer to the device object or 0 if not found
	CDevice *GetDevice (const char *pPrefix, unsigned nIndex, boolean bBlockDevice);

	/// \brief Enumerate all devices, or all devices of a specified prefix
	/// \param callback A callback to be invoked for each matching device
	/// \param arg A user define pointer that will back passed to the callback
	/// \return false if the enumeration was cancelled by the callback returning false
	boolean EnumerateDevices (
		boolean (*callback)(CDevice* pDevice, const char* name, boolean bBlockDevice, void* arg), 
		void* arg
	);

	/// \brief Generate device listing
	/// \param pTarget Device to be used for output
	void ListDevices (CDevice *pTarget);

	/// \return The single CDeviceNameService instance of the system
	static CDeviceNameService *Get (void);

private:
	TDeviceInfo *m_pList;

	CSpinLock m_SpinLock;

	static CDeviceNameService *s_This;
};

#endif
