#ifndef CIRCLE_ADDON_ALPHA_FILEIO_H_
# define CIRCLE_ADDON_ALPHA_FILEIO_H_

# include <alpha/fileio.h>

///
/// GDB File I/O functions.
/// These functions are *subsets* of the actual syscalls, documented at
/// https://sourceware.org/gdb/onlinedocs/gdb/List-of-Supported-Calls.html
///
class FileIO {
public:
  static fio_uint32_t m_nErrno;

  static int Open (const char* pFile, int nFlags, int nMode = 0);
  static int Write (int nFd, const void *pBuffer, unsigned nCount);
  static int Read (int nFd, void *pBuffer, unsigned nCount);
  static int FStat (int nFd, struct fio_stat* pStat);
  static bool IsTTY (int nFd);
  static int LSeek (int nFd, int nOffset, int nWhence);
  static int Close (int nFd);
  static int Stat (const char* pFile, struct fio_stat* pStat);
  static int Rename (const char* pOldPath, const char* pNewPath);
  static int Unlink (const char* pFile);
  static int GetTimeOfDay (struct fio_timeval* pTimeval);
  static int System (const char* pCommand);
};

#endif /* !CIRCLE_ADDON_ALPHA_FILEIO_H_ */
