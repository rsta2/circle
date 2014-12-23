//
// partitionmanager.h
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
#ifndef _circle_fs_partitionmanager_h
#define _circle_fs_partitionmanager_h

#include <circle/device.h>
#include <circle/fs/partition.h>
#include <circle/string.h>
#include <circle/types.h>

#define MAX_PARTITIONS		4

class CPartitionManager
{
public:
	CPartitionManager (CDevice *pDevice, const char *pDeviceName);
	~CPartitionManager (void);

	boolean Initialize (void);

private:
	CDevice *m_pDevice;
	CString  m_DeviceName;

	CPartition *m_pPartition[MAX_PARTITIONS];
};

#endif
