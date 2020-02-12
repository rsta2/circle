//
// profiler.h
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
#ifndef _profile_profiler_h
#define _profile_profiler_h

#include <circle/fs/fat/fatfs.h>
#include <circle/types.h>
#include <fatfs/ff.h>

extern u8 _start, _etext;

class CProfiler		/// A software profiler
{
public:
	/// \param nTextStart Start address of the code to be profiled
	/// \param nTextEnd End address of the code to be profiled
	CProfiler (uintptr nTextStart = (uintptr) &_start,
		   uintptr nTextEnd = (uintptr) &_etext);

	~CProfiler (void);

	/// \brief Stop profiling and save the results to the file "GMON.OUT"
	/// \param pPartitionName Name of the partition to be used (default: SD card)
	/// \note This method uses the class CFATFileSystem.\n
	///	  The file system is mounted and unmounted automatically.
	void SaveResults (const char *pPartitionName = "emmc1-1");

	/// \brief Stop profiling and save the results to the file "GMON.OUT"
	/// \param pFileSystem Pointer to the file system object to be used
	/// \note The file system must already be mounted before.
	void SaveResults (CFATFileSystem *pFileSystem);

	/// \brief Stop profiling and save the results to the file "gmon.out"
	/// \param pFileSystem Pointer to the FatFs file system struct to be used
	/// \param pDriveName Name of the drive to be used (default: SD card)
	/// \note This method uses the FatFs module.\n
	///	  The file system must already be mounted before.
	void SaveResults (FATFS *pFileSystem, const char *pDriveName = "SD:");
};

#endif
