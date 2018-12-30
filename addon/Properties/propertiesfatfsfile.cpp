//
// propertiesfile.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2018  R. Stange <rsta2@o2online.de>
//
// FatFs support by Steve Maynard
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
#include <Properties/propertiesfatfsfile.h>
#include <assert.h>

CPropertiesFatFsFile::CPropertiesFatFsFile (const char *pFileName, FATFS *pFileSystem)
:	m_FileName (pFileName)
{
}

CPropertiesFatFsFile::~CPropertiesFatFsFile (void)
{
}

boolean CPropertiesFatFsFile::Load (void)
{
	FRESULT Result;
	FIL File;
	unsigned nBytesRead;

	RemoveAll ();

	ResetErrorLine ();

	assert (m_FileName.GetLength () > 0);
	Result = f_open (&File, m_FileName, FA_READ | FA_OPEN_EXISTING);
	if (Result != FR_OK)
	{
		return FALSE;
	}

	char Buffer[500];
	do
	{
		Result = f_read (&File, Buffer, sizeof Buffer, &nBytesRead);
		if (Result != FR_OK)
		{
			return FALSE;
		}

		if (nBytesRead > 0)
		{
			for (unsigned i = 0; i < nBytesRead; i++)
			{
				if (!Parse (Buffer[i]))
				{
					f_close (&File);

					return FALSE;
				}
			}
		}
	} while (nBytesRead > 0);

	f_close (&File);

	return Parse ('\n');		// fake an empty line at file end
}

boolean CPropertiesFatFsFile::Save (void)
{
	FRESULT Result;
	FIL File;
	unsigned nBytesWritten;

	assert (m_FileName.GetLength () > 0);
	Result = f_open (&File, m_FileName, FA_WRITE | FA_CREATE_ALWAYS);
	if (Result != FR_OK)
	{
		return FALSE;
	}

	boolean bContinue = GetFirst ();
	while (bContinue)
	{
		CString Line;
		Line.Format ("%s=%s\n", GetName (), GetValue ());

		if ( (Result = f_write (&File, (const char *) Line, Line.GetLength (),
			&nBytesWritten)) != FR_OK || nBytesWritten != Line.GetLength ())
		{
			f_close (&File);

			return FALSE;
		}

		bContinue = GetNext ();
	}

	return f_close (&File) == FR_OK;
}
