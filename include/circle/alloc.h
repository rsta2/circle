//
// alloc.h
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
#ifndef _alloc_h
#define _alloc_h

//#define MEM_DEBUG

#ifdef __cplusplus
extern "C" {
#endif

void mem_init (unsigned long ulBase, unsigned long ulSize);

unsigned long mem_get_size (void);

void *malloc (unsigned long ulSize);	// resulting block is always 16 bytes aligned
void free (void *pBlock);

void *palloc (void);		// returns 4K page (aligned)
void pfree (void *pPage);

#ifdef MEM_DEBUG

void mem_info (void);

#endif

#ifdef __cplusplus
}
#endif

#endif
