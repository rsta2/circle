//
// glibc_compat.h
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
#ifndef _profile_glibc_compat_h
#define _profile_glibc_compat_h

#include <circle/fs/fat/fatfs.h>
#include <fatfs/ff.h>
#include <circle/types.h>

#define __BEGIN_DECLS
#define __END_DECLS
#define __THROW

#define attribute_hidden
#define libc_hidden_def(symbol)
#define libc_hidden_proto(symbol)
#define weak_alias(symbol, alias)

#define NULL	0

typedef unsigned char	u_char;
typedef unsigned short	u_short;
typedef unsigned int	u_int;
typedef unsigned long	u_long;

struct iovec
{
	void *iov_base;
	size_t iov_len;
};

#define STDERR_FILENO	0

#define O_CREAT		(1 << 0)
#define O_TRUNC		(1 << 1)
#define O_WRONLY	(1 << 2)
#define O_NOFOLLOW	(1 << 3)

void __set_nocancel_filesystem (CFATFileSystem *fs);
void __set_nocancel_filesystem (FATFS *fs, const char *drive);

int __open_nocancel (const char *name, unsigned mode, unsigned umask);
void __writev_nocancel_nostatus (int fd, const struct iovec *vec, size_t len);
void __write_nocancel (int fd, const void *buf, size_t len);
void __close_nocancel_nostatus (int fd);

static inline bool atomic_compare_and_exchange_bool_acq (long *mem, long newval, long oldval)
{
	long expected = oldval;

	return !__atomic_compare_exchange_n (mem, &expected, newval, false,
#ifdef ARM_ALLOW_MULTI_CORE
					     __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST
#else
					     __ATOMIC_RELAXED, __ATOMIC_RELAXED
#endif
		);
}

static inline int ffs (int i)
{
	int mask = 1;
	for (int n = 1; n <= 32; n++, mask <<= 1)
	{
		if (i & mask)
		{
			return n;
		}
	}

	return 0;
}

#endif
