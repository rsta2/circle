//
// profile.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2020  R. Stange <rsta2@o2online.de>
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
#include <profile/profiler.h>
#include <profile/gmon.h>
#include <circle/devicenameservice.h>
#include <circle/sysconfig.h>
#include <circle/logger.h>

#ifdef ARM_ALLOW_MULTI_CORE
	#error Multi-core programs are not supported by this profile library!
#endif

static const char From[] = "prof";

CProfiler::CProfiler (uintptr nTextStart, uintptr nTextEnd)
{
	__monstartup (nTextStart, nTextEnd);
}

CProfiler::~CProfiler (void)
{
}

void CProfiler::SaveResults (const char *pPartitionName)
{
	CDevice *pPartition = CDeviceNameService::Get ()->GetDevice (pPartitionName, TRUE);
	if (pPartition == 0)
	{
		CLogger::Get ()->Write (From, LogError, "Partition not found: %s", pPartitionName);

		return;
	}

	CFATFileSystem FileSystem;
	if (!FileSystem.Mount (pPartition))
	{
		CLogger::Get ()->Write (From, LogError, "Cannot mount partition: %s", pPartitionName);

		return;
	}

	SaveResults (&FileSystem);

	FileSystem.UnMount ();
}

void CProfiler::SaveResults (CFATFileSystem *pFileSystem)
{
	__set_nocancel_filesystem (pFileSystem);

	_mcleanup ();

	CLogger::Get ()->Write (From, LogDebug, "Profiling results saved");
}

void CProfiler::SaveResults (FATFS *pFileSystem, const char *pDriveName)
{
	__set_nocancel_filesystem (pFileSystem, pDriveName);

	_mcleanup ();

	CLogger::Get ()->Write (From, LogDebug, "Profiling results saved");
}
