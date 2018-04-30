#include "fileio.h"
#include <alpha/fileio.h>
#include <circle/devicenameservice.h>
#include <assert.h>

fio_uint32_t FileIO::m_nErrno = 0;

int FileIO::Open (const char* pFile, int nFlags, int nMode)
{
  fio_int32_t rc;
  FILEIO_OPEN(pFile, nFlags, nMode, &m_nErrno, &rc);
  return rc;
}

int FileIO::Write (int nFd, const void *pBuffer, unsigned nCount)
{
  fio_int32_t rc;
  FILEIO_WRITE(nFd, pBuffer, nCount, &m_nErrno, &rc);
  return rc;
}

int FileIO::Read (int nFd, void *pBuffer, unsigned nCount)
{
  fio_int32_t rc;
  FILEIO_READ(nFd, pBuffer, nCount, &m_nErrno, &rc);
  return rc;
}

int FileIO::FStat (int nFd, struct fio_stat* pStat)
{
  fio_int32_t rc;
  FILEIO_FSTAT(nFd, pStat, &m_nErrno, &rc);
  return rc;
}

bool FileIO::IsTTY (int nFd)
{
  fio_int32_t rc;
  FILEIO_ISATTY(nFd, &m_nErrno, &rc);
  return rc == 1;
}

int FileIO::LSeek (int nFd, int nOffset, int nWhence)
{
  fio_int32_t rc;
  FILEIO_LSEEK(nFd, nOffset, nWhence, &m_nErrno, &rc);
  return rc;
}

int FileIO::Close (int nFd)
{
  fio_int32_t rc;
  FILEIO_CLOSE(nFd, &m_nErrno, &rc);
  return rc;
}

int FileIO::Stat (const char* pFile, struct fio_stat* pStat)
{
  fio_int32_t rc;
  FILEIO_STAT(pFile, pStat, &m_nErrno, &rc);
  return rc;
}

int FileIO::Rename (const char* pOldPath, const char* pNewPath)
{
  fio_int32_t rc;
  FILEIO_RENAME(pOldPath, pNewPath, &m_nErrno, &rc);
  return rc;
}

int FileIO::Unlink (const char* pFile)
{
  fio_int32_t rc;
  FILEIO_UNLINK(pFile, &m_nErrno, &rc);
  return rc;
}

int FileIO::GetTimeOfDay (struct fio_timeval* pTimeval)
{
  fio_int32_t rc;
  FILEIO_GETTIMEOFDAY(pTimeval, 0, &m_nErrno, &rc);
  return rc;
}

int FileIO::System (const char* pCommand)
{
  fio_int32_t rc;
  FILEIO_SYSTEM(pCommand, &m_nErrno, &rc);
  return rc;
}
