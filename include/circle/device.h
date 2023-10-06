//
// device.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2023  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_device_h
#define _circle_device_h

#include <circle/ptrlist.h>
#include <circle/types.h>

class CDevice;

typedef void TDeviceRemovedHandler (CDevice *pDevice, void *pContext);

class CDevice		/// Base class for all devices
{
public:
	CDevice (void);
	virtual ~CDevice (void);

	/// \param pBuffer Buffer, where read data will be placed
	/// \param nCount Maximum number of bytes to be read
	/// \return Number of read bytes or < 0 on failure
	virtual int Read (void *pBuffer, size_t nCount);

	/// \param pBuffer Buffer, from which data will be fetched for write
	/// \param nCount Number of bytes to be written
	/// \return Number of written bytes or < 0 on failure
	virtual int Write (const void *pBuffer, size_t nCount);

	/// \param ullOffset Byte offset from start
	/// \return The resulting offset, (u64) -1 on error
	/// \note Supported by block devices only
	virtual u64 Seek (u64 ullOffset);

	/// \return Total byte size of a block device, (u64) -1 on error
	/// \note Supported by block devices only
	virtual u64 GetSize (void) const;

	/// \param ulCmd The IOCtl command to invoke
	/// \param pData Depends on command, used to return command specific data
	/// \return Zero on success, or error code on failure
	virtual int IOCtl (unsigned long ulCmd, void *pData);

	/// \return TRUE on successful device removal
	virtual boolean RemoveDevice (void);

public:
	typedef void *TRegistrationHandle;

	/// \param pHandler Handler gets called, when device is destroyed
	/// \param pContext Context pointer handed over to the handler
	/// \return Handle to be handed over to UnregisterRemovedHandler()
	/// \note Can be called multiple times. Handlers will be called in reverse order.
	/// \note Calling this with pHandler = 0 to unregister is not supported any more.
	TRegistrationHandle RegisterRemovedHandler (TDeviceRemovedHandler *pHandler,
						    void *pContext = 0);

	/// \param hRegistration Handle returned by RegisterRemovedHandler()
	void UnregisterRemovedHandler (TRegistrationHandle hRegistration);

private:
	CPtrList m_RemovedHandlerList;
};

#endif
