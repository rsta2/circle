//
// propertiesfile.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2018  R. Stange <rsta2@o2online.de>
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
#include <Properties/propertiesfile.h>
#include <assert.h>

CPropertiesFile::CPropertiesFile (const char *pFileName, CFATFileSystem *pFileSystem)
:	m_pFileSystem (pFileSystem),
	m_FileName (pFileName)
{
}

CPropertiesFile::~CPropertiesFile (void)
{
	m_pFileSystem = 0;
}

boolean CPropertiesFile::Load (void)
{
	RemoveAll ();

	ResetErrorLine ();

	assert (m_pFileSystem != 0);
	assert (m_FileName.GetLength () > 0);
	unsigned hFile = m_pFileSystem->FileOpen (m_FileName);
	if (hFile == 0)
	{
		return FALSE;
	}

	char Buffer[500];
	unsigned nResult;
	while ((nResult = m_pFileSystem->FileRead (hFile, Buffer, sizeof Buffer)) > 0)
	{
		if (nResult == FS_ERROR)
		{
			m_pFileSystem->FileClose (hFile);

			return FALSE;
		}

		for (unsigned i = 0; i < nResult; i++)
		{
			if (!Parse (Buffer[i]))
			{
				m_pFileSystem->FileClose (hFile);

				return FALSE;
			}
		}
	}

	m_pFileSystem->FileClose (hFile);

	return Parse ('\n');		// fake an empty line at file end
}

boolean CPropertiesFile::Save (void)
{
	assert (m_pFileSystem != 0);
	assert (m_FileName.GetLength () > 0);
	unsigned hFile = m_pFileSystem->FileCreate (m_FileName);
	if (hFile == 0)
	{
		return FALSE;
	}

	boolean bContinue = GetFirst ();
	while (bContinue)
	{
		CString Line;
		Line.Format ("%s=%s\n", GetName (), GetValue ());

		if (   m_pFileSystem->FileWrite (hFile, (const char *) Line, Line.GetLength ())
		    != Line.GetLength ())
		{
			m_pFileSystem->FileClose (hFile);

			return FALSE;
		}

		bContinue = GetNext ();
	}
	
	return m_pFileSystem->FileClose (hFile) ? TRUE : FALSE;
}
