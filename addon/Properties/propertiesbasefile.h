//
// propertiesbasefile.h
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
#ifndef _Properties_propertiesbasefile_h
#define _Properties_propertiesbasefile_h

#include <Properties/properties.h>
#include <circle/types.h>

#define MAX_PROPERTY_NAME_LENGTH	100
#define MAX_PROPERTY_VALUE_LENTGH	500

class CPropertiesBaseFile : public CProperties
{
public:
	CPropertiesBaseFile (void);
	virtual ~CPropertiesBaseFile (void);

	virtual boolean Load (void) = 0;	// may be partially loaded on error
	virtual boolean Save (void) = 0;	// overwrites existing file

	// if Load() fails this returns the line where the error occurred
	unsigned GetErrorLine (void) const;

protected:
	void ResetErrorLine (void);

	boolean Parse (char chChar);

	static boolean IsValidNameChar (char chChar);

private:
	unsigned m_nState;
	unsigned m_nIndex;	// for the arrays below
	char m_PropertyName[MAX_PROPERTY_NAME_LENGTH];
	char m_PropertyValue[MAX_PROPERTY_VALUE_LENTGH];

	unsigned m_nLine;
};

#endif
