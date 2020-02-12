//
// glibc_compat.cpp
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
#include <profile/glibc_compat.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <assert.h>

#define FATFS_FILE_DESCRIPTOR	1		// only one open file supported

static CFATFileSystem *s_pFileSystem = 0;

static FATFS *s_pFATFS = 0;
static char s_FATFSDrive[10];
static FIL *s_pFile = 0;

static const char From[] = "prof";

void __set_nocancel_filesystem (CFATFileSystem *fs)
{
	assert (fs != 0);
	s_pFileSystem = fs;
}

void __set_nocancel_filesystem (FATFS *fs, const char *drive)
{
	assert (fs != 0);
	s_pFATFS = fs;

	assert (drive != 0);
	strncpy (s_FATFSDrive, drive, sizeof s_FATFSDrive);
	s_FATFSDrive[sizeof s_FATFSDrive-1] = '\0';
}

int __open_nocancel (const char *name, unsigned mode, unsigned umask)
{
	assert (name != 0);

	if ((mode & (O_CREAT | O_TRUNC)) != (O_CREAT | O_TRUNC))
	{
		return -1;
	}

	if (s_pFileSystem != 0)
	{
		unsigned hFile = s_pFileSystem->FileCreate (name);

		return hFile > 0 ? (int) hFile : -1;
	}
	else
	{
		assert (s_pFATFS != 0);

		char path[80];
		if (strlen (s_FATFSDrive) + strlen (name) > sizeof path-1)
		{
			return -1;
		}

		strcpy (path, s_FATFSDrive);
		strcat (path, name);

		if (s_pFile != 0)
		{
			return -1;
		}

		s_pFile = new FIL;
		assert (s_pFile != 0);

		if (f_open (s_pFile, path, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK)
		{
			return FATFS_FILE_DESCRIPTOR;
		}
		else
		{
			delete s_pFile;
			s_pFile = 0;

			return -1;
		}
	}
}

void __writev_nocancel_nostatus (int fd, const struct iovec *vec, size_t len)
{
	assert (vec != 0);

	for (unsigned i = 0; i < len; i++)
	{
		__write_nocancel (fd, vec[i].iov_base, vec[i].iov_len);
	}
}

void __write_nocancel (int fd, const void *buf, size_t len)
{
	assert (buf != 0);
	assert (len > 0);

	if (fd == STDERR_FILENO)
	{
		char msg[100];
		if (len > sizeof msg-1)
		{
			len = sizeof msg-1;
		}

		memcpy (msg, buf, len);
		msg[len] = '\0';

		if (   len > 1
		    && msg[len-1] == '\n')
		{
			msg[len-1] = '\0';
		}

		CLogger::Get ()->Write (From, LogError, msg);

		return;
	}

	if (s_pFileSystem != 0)
	{
		assert (len < (unsigned) -1);

		if (s_pFileSystem->FileWrite ((unsigned) fd, buf, (unsigned) len) != (unsigned) len)
		{
			CLogger::Get ()->Write (From, LogError, "Write failed");
		}
	}
	else
	{
		assert (s_pFATFS != 0);

		unsigned byteswritten;
		if (   fd != FATFS_FILE_DESCRIPTOR
		    || s_pFile == 0
		    || f_write (s_pFile, (const char *) buf, len, &byteswritten) != FR_OK
		    || byteswritten != len)
		{
			CLogger::Get ()->Write (From, LogError, "Write failed");
		}
	}
}

void __close_nocancel_nostatus (int fd)
{
	if (s_pFileSystem != 0)
	{
		s_pFileSystem->FileClose ((unsigned) fd);
	}
	else
	{
		assert (s_pFATFS != 0);

		if (   fd == FATFS_FILE_DESCRIPTOR
		    && s_pFile != 0)
		{
			f_close (s_pFile);

			delete s_pFile;
			s_pFile = 0;
		}
	}
}
