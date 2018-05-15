#include "gdbconsole.h"
#include "fileio.h"
#include <circle/devicenameservice.h>

GDBConsole::GDBConsole (const char* pDeviceName)
{
  CDeviceNameService::Get ()->AddDevice (pDeviceName, this, FALSE);
}

int GDBConsole::Write (const void *pBuffer, unsigned nCount)
{
  return FileIO::Write(1, pBuffer, nCount);
}

int GDBConsole::Read (void *pBuffer, unsigned nCount)
{
  return FileIO::Write(0, pBuffer, nCount);
}
