//
// version.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_version_h
#define _circle_version_h

#ifdef __cplusplus
extern "C" char circle_version_string[];
#else
extern char circle_version_string[];
#endif

#define CIRCLE_NAME			"Circle"

#define CIRCLE_MAJOR_VERSION		(__circle__ / 10000)
#define CIRCLE_MINOR_VERSION		(__circle__ / 100 % 100)
#define CIRCLE_PATCH_VERSION		(__circle__ % 100)
#define CIRCLE_VERSION_STRING		circle_version_string

#define OS_NAME				CIRCLE_NAME
#define OS_VERSION			CIRCLE_VERSION_STRING

#endif
