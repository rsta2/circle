//
// fsdef.h
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
#ifndef _circle_fs_fsdef_h
#define _circle_fs_fsdef_h

#define FS_BLOCK_SIZE	512
#define FS_BLOCK_MASK	(FS_BLOCK_SIZE-1)
#define FS_BLOCK_SHIFT	9

#define FS_TITLE_LEN	12				// maximum of all file systems

struct TDirentry					// for directory listing
{
	char		chTitle[FS_TITLE_LEN+1];	// 0-terminated
	unsigned	nSize;
	unsigned	nAttributes;
};

#define FS_ATTRIB_NONE		0x00
#define FS_ATTRIB_SYSTEM	0x01
#define FS_ATTRIB_EXECUTABLE	0x02

struct TFindCurrentEntry			// current position for directory listing
{
	unsigned	nEntry;
	unsigned	nCluster;
};

#define FS_ERROR	0xFFFFFFFF

#endif
