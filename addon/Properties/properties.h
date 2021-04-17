//
// properties.h
//
// Base class for configuration properties
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2021  R. Stange <rsta2@o2online.de>
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
#ifndef _Properties_properties_h
#define _Properties_properties_h

#include <circle/ptrarray.h>
#include <circle/types.h>

struct TPropertyPair;

class CProperties
{
public:
	CProperties (void);
	~CProperties (void);

	boolean IsSet (const char *pPropertyName) const;

	// resulting string is available as long this class is instanciated
	const char *GetString (const char *pPropertyName, const char *pDefault = 0) const;
	unsigned GetNumber (const char *pPropertyName, unsigned nDefault = 0) const;
	int GetSignedNumber (const char *pPropertyName, int nDefault = 0) const;
	// returns 0 if not set, result is valid until the next call of this method
	const u8 *GetIPAddress (const char *pPropertyName);

	// existing properties will be replaced
	void SetString (const char *pPropertyName, const char *pValue);
	void SetNumber (const char *pPropertyName, unsigned nValue, unsigned nBase = 10);
	void SetSignedNumber (const char *pPropertyName, int nValue);	// base is always 10
	void SetIPAddress (const char *pPropertyName, const u8 *pAddress);

	void RemoveAll (void);

#ifndef NDEBUG
	void Dump (const char *pSource = "properties") const;
#endif

protected:
	// existing properties will be ignored
	void AddProperty (const char *pPropertyName, const char *pValue);

	// iterating over all properties
	boolean GetFirst (void);
	boolean GetNext (void);

	// getting name and value at current position
	const char *GetName (void) const;
	const char *GetValue (void) const;

private:
	TPropertyPair *Lookup (const char*pPropertyName) const;

private:
	CPtrArray m_PropArray;
	unsigned m_nGetIndex;

	u8 m_IPAddress[4];
};

#endif
