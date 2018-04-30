//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016  R. Stange <rsta2@o2online.de>
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
#include "kernel.h"
#include <circle/timer.h>
#include <circle/util.h>
#include <addon/alpha/fileio.h>

#define LOG_EXAMPLE(FMT, ...)                                   \
  do {                                                          \
    CString str;                                                \
    str.Format(EXAMPLE_LOGGER_PREFIX FMT, ##__VA_ARGS__);       \
    m_Logger.Write ("kernel", LogNotice, (const char*) str);    \
  } while (0)

#define EXAMPLE_LOGGER_PREFIX                         \
  "\x1b[1;46m Exemple " STR(__COUNTER__) " \x1b[0m "

#define STR(a) STR_(a)
#define STR_(a) #a

#define PANIC(FMT, ...)                                       \
  do {                                                        \
    CString str;                                              \
    str.Format(FMT, ##__VA_ARGS__);                           \
    m_Logger.Write ("kernel", LogPanic, (const char*) str);   \
  } while (0)

#define PRINTF(FMT, ...)                                        \
  do {                                                          \
    CString str;                                                \
    str.Format(FMT, ##__VA_ARGS__);                             \
    m_GDBConsole.Write ((const char*) str, str.GetLength());    \
  } while (0)

CKernel::CKernel (void)
: m_Timer (&m_Interrupt),
  m_Logger (LogDebug, &m_Timer)
{
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
  CDevice *pTarget = m_DeviceNameService.GetDevice ("gdb", FALSE);
  if (pTarget == 0)
    return FALSE;

  return m_Logger.Initialize (pTarget);
}

TShutdownMode CKernel::Run (void)
{
  m_Logger.Write ("kernel", LogNotice, "Hello from CLogger");
	m_Logger.Write ("kernel", LogNotice, "Compile time: " __DATE__ " " __TIME__);

  // open
  const char* a_file = "gdb-fileio-example.svg";
  LOG_EXAMPLE("open(): create and open file `%s`", a_file);
  int fd = FileIO::Open (a_file,
                         FILEIO_O_WRONLY | FILEIO_O_CREAT | FILEIO_O_TRUNC,
                         FILEIO_S_IWUSR | FILEIO_S_IRUSR | FILEIO_S_IRGRP | FILEIO_S_IROTH);
  if (fd == -1) {
    PANIC("open(): %d", FileIO::m_nErrno);
  }

  // write
  LOG_EXAMPLE("write(): write an SVG image in `%s`", a_file);
  extern const char* farjump_logo_svg;
  if (FileIO::Write (fd, farjump_logo_svg, strlen(farjump_logo_svg)) == -1) {
    PANIC("write(): %s", FileIO::m_nErrno);
  }

  // close
  LOG_EXAMPLE("close(): close the file descriptor of `%s`", a_file);
  if (FileIO::Close (fd) == -1) {
    PANIC("close(): %s", FileIO::m_nErrno);
  }

  // rename
  const char* new_filename = "farjump.svg";
  LOG_EXAMPLE("rename(): rename `%s` into `%s`", a_file, new_filename);
  if (FileIO::Rename (a_file, new_filename) != 0) {
    PANIC("rename(): %s", FileIO::m_nErrno);
  }

  // system
  LOG_EXAMPLE("system(): run a shell script to check the existence of file `%s`", new_filename);
  CString cmd;
  cmd.Format("set -x && echo using system && test -e %s", new_filename);
  int rc = FileIO::System ((const char*) cmd);
  if (rc != 0) {
    PANIC("system(): \"%s\" returned `%d`", cmd, rc);
  }

  // stat
  LOG_EXAMPLE("stat(): get and print the file status of `%s`", new_filename);
  struct fio_stat sb;
  if (FileIO::Stat (new_filename, &sb) == -1) {
    PANIC("stat(): %s", FileIO::m_nErrno);
  }
  PrintStat(&sb);

  // open
  LOG_EXAMPLE("open(): open file `%s` in read-only mode", new_filename);
  fd = FileIO::Open (new_filename, FILEIO_O_RDONLY);
  if (fd == -1) {
    PANIC("open(): %s", FileIO::m_nErrno);
  }

  // read
  LOG_EXAMPLE("read(): partly read file `%s` and compare the retrieved data with the original data", new_filename);
  char buffer[126];
  int n = FileIO::Read (fd, buffer, sizeof (buffer));
  if (n == -1) {
    PANIC("read(): %s", FileIO::m_nErrno);
  }
  if (n == 0) {
    PANIC("read() returned 0 bytes");
  }
  if (strncmp(buffer, farjump_logo_svg, n) != 0) {
    PANIC("bytes read from file `%s` are different from the original data", new_filename);
  }

  // lseek
  LOG_EXAMPLE("lseek(): partly read file `%s` and compare the retrieved data with the original data", new_filename);
  int offset = FileIO::LSeek (fd, 33, FILEIO_SEEK_CUR);
  if (offset == -1) {
    PANIC("lseek(): %s", FileIO::m_nErrno);
  }
  PRINTF("file cursor moved at %d", offset);
  n = FileIO::Read (fd, buffer, sizeof (buffer));
  if (n == -1) {
    PANIC("read(): %s", FileIO::m_nErrno);
  }
  if (n == 0) {
    PANIC("read() returned 0 bytes");
  }
  if (strncmp(buffer, &farjump_logo_svg[offset], n) != 0) {
    PANIC("bytes read from file `%s` are different from the original data", new_filename);
  }

  // isatty
  LOG_EXAMPLE("isatty(): check that `%s` is indeed not a tty", new_filename);
  if (FileIO::IsTTY (fd)) {
    PANIC("isatty() did not return 0: %s", FileIO::m_nErrno);
  }

  // close
  LOG_EXAMPLE("close(): closing file `%s`", new_filename);
  if (FileIO::Close (fd) == -1) {
    PANIC("close(): %s", FileIO::m_nErrno);
  }

  // isatty
  LOG_EXAMPLE("isatty(): check that stdin is a TTY");
  if (!FileIO::IsTTY (0)) {
    PANIC("isatty() did not return true: %s", FileIO::m_nErrno);
  }

  // unlink
  LOG_EXAMPLE("unlink(): removing file `%s`", new_filename);
  if (FileIO::Unlink (new_filename) == -1) {
    PANIC("unlink(): %s", FileIO::m_nErrno);
  }

  // gettimeofday
  LOG_EXAMPLE("gettimeofday(): display current date and time");
  struct fio_timeval tv;
  n = FileIO::GetTimeOfDay (&tv);
  if (n != 0) {
    PANIC("gettimeofday(): return code `%d`", n);
  }
  PRINTF("GMT-0 time in seconds is %d\n", tv.ftv_sec);

  // File I/Os with special files
  const char* dev = "/dev/random";
  LOG_EXAMPLE("using the File I/O extension with special file `%s`", dev);

  PRINTF("open(): open special file `%s`\n", dev);
  fd = FileIO::Open (dev, FILEIO_O_RDONLY);
  if (fd == -1) {
    PANIC("open(): %s", FileIO::m_nErrno);
  }

  PRINTF("fstat(): get the file status of `%s`\n", dev);
  if (FileIO::FStat (fd, &sb) == -1) {
    PANIC("fstat(): %s", FileIO::m_nErrno);
  }
  PrintStat(&sb);

  PRINTF("read(): read some data from special file `%s`\n", dev);
  int x = 0;
  n = FileIO::Read (fd, &x, sizeof (x));
  if (n == -1) {
    PANIC("read(): %s", FileIO::m_nErrno);
  }
  if (n == 0) {
    PANIC("read() returned 0 bytes");
  }
  buffer[n] = 0;
  PRINTF("read %d bytes `%d`", n, x);

  if (FileIO::Close (fd) == -1 ) {
    PANIC("close(): %s", FileIO::m_nErrno);
  }

  // File I/Os with special files
  dev = "/dev/stdout";
  LOG_EXAMPLE("using the File I/O extension with special file `%s`", dev);

  PRINTF("open(): open special file `%s`\n", dev);
  fd = FileIO::Open (dev, FILEIO_O_WRONLY);
  if (fd == -1) {
    PANIC("open(): %s", FileIO::m_nErrno);
  }

  PRINTF("fstat(): get the file status of `%s`\n", dev);
  if (FileIO::FStat (fd, &sb) == -1) {
    PANIC("fstat(): %s", FileIO::m_nErrno);
  }
  PrintStat(&sb);

  const char data[] = "Hello, File I/O!\n";
  PRINTF("write(): write `%s` to special file `%s`\n", data, dev);
  n = FileIO::Write (fd, data, sizeof (data));
  if (n == -1) {
    PANIC("write(): %s", FileIO::m_nErrno);
  }
  if (n == 0) {
    PANIC("write() returned 0 bytes");
  }

  if (FileIO::Close (fd) == -1 ) {
    PANIC("close(): %s", FileIO::m_nErrno);
  }

	return ShutdownReboot;
}

#define S_IFMT 0170000
void CKernel::PrintStat (struct fio_stat *sb) {
  PRINTF("File mode:                %x\n");
  PRINTF("I-node number:            %d\n", (long) sb->fst_ino);
  PRINTF("Device ID:                0x%x\n", (long) sb->fst_rdev);
  PRINTF("Container Device ID:      0x%x\n", (long) sb->fst_dev);
  PRINTF("Mode:                     %o (octal)\n",
         (unsigned long) sb->fst_mode);
  PRINTF("Link count:               %d\n", (long) sb->fst_nlink);
  PRINTF("Ownership:                UID=%d   GID=%d\n",
         (long) sb->fst_uid, (long) sb->fst_gid);
  PRINTF("Preferred I/O block size: %d bytes\n",
         (long) sb->fst_blksize);
  PRINTF("File size:                %d bytes\n",
         (long long) sb->fst_size);
  PRINTF("Blocks allocated:         %d\n",
         (long long) sb->fst_blocks);
  PRINTF("Last status change:       %d\n", sb->fst_ctime);
  PRINTF("Last file access:         %d\n", sb->fst_atime);
  PRINTF("Last file modification:   %d\n", sb->fst_mtime);
}
