//
// config.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2026  R. Stange <rsta2@gmx.net>
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
#ifndef _config_h
#define _config_h

#define PIPE_SIZE	0x4000	// Size of the pipe FIFO (actually it is one byte smaller)
#define ATOMIC_WRITE	512	// Number of bytes to be send atomically (PIPE_BUF)

#define NUM_WRITERS	2	// Number of writing tasks
#define NUM_READERS	2	// Number of reading tasks

#define NON_BLOCKING	0	// Set to 1 for non-blocking pipe mode
#define CHECK_STATUS	1	// Set to 1 for checking pipe status before read or write

#define MAX_WRITES	20	// Maximum number of writes
#define MAX_READS	20	// Maximum number of reads

#define MAX_WRITE_BYTES	16	// Maximum number of bytes to write at once
#define MAX_READ_BYTES	32	// Maximum number of bytes to read at once

#define MAX_SLEEP_MS	10	// Maximum milliseconds for writer to sleep between writes

#endif
