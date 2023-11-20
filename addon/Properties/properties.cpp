//
// properties.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2023  R. Stange <rsta2@o2online.de>
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
#include <Properties/properties.h>
#include <circle/ptrarray.h>
#include <circle/util.h>
#include <circle/logger.h>
#include <assert.h>

struct TSection
{
	CString Name;
	CPtrArray PropArray;
};

struct TPropertyPair
{
	char *pName;
	char *pValue;
};

const char CProperties::DefaultSection[] = "";

CProperties::CProperties (void)
:	m_CurrentSectionName (DefaultSection),
	m_pCurrentSection (nullptr),
	m_pGetSection (nullptr),
	m_nGetIndex (0)
{
}

CProperties::~CProperties (void)
{
	RemoveAll ();
}

void CProperties::SelectSection (const char *pSectionName)
{
	m_CurrentSectionName = pSectionName;

	m_pCurrentSection = LookupSection (pSectionName);
}

boolean CProperties::IsSet (const char *pPropertyName) const
{
	return Lookup (pPropertyName) != 0;
}

const char *CProperties::GetString (const char *pPropertyName, const char *pDefault) const
{
	TPropertyPair *pProperty = Lookup (pPropertyName);
	if (pProperty == 0)
	{
		return pDefault;
	}

	return pProperty->pValue;
}

unsigned CProperties::GetNumber (const char *pPropertyName, unsigned nDefault) const
{
	TPropertyPair *pProperty = Lookup (pPropertyName);
	if (pProperty == 0)
	{
		return nDefault;
	}

	assert (pProperty->pValue != 0);
	char *pEndPtr = 0;
	unsigned long ulResult = strtoul (pProperty->pValue, &pEndPtr, 0);
	if (pEndPtr != 0 && *pEndPtr != '\0')
	{
		return nDefault;
	}

	if (ulResult > (unsigned) -1)
	{
		return nDefault;
	}

	return (unsigned) ulResult;
}

int CProperties::GetSignedNumber (const char *pPropertyName, int nDefault) const
{
	TPropertyPair *pProperty = Lookup (pPropertyName);
	if (pProperty == 0)
	{
		return nDefault;
	}

	const char *pValue = pProperty->pValue;
	assert (pValue != 0);

	boolean bMinus = *pValue == '-';
	if (bMinus)
	{
		pValue++;
	}

	char *pEndPtr = 0;
	unsigned long ulResult = strtoul (pValue, &pEndPtr, 0);
	if (pEndPtr != 0 && *pEndPtr != '\0')
	{
		return nDefault;
	}

	if (ulResult >= 0x80000000)
	{
		return nDefault;
	}

	int nResult = (int) ulResult;

	return bMinus ? -nResult : nResult;
}

const u8 *CProperties::GetIPAddress (const char *pPropertyName)
{
	TPropertyPair *pProperty = Lookup (pPropertyName);
	if (pProperty == 0)
	{
		return 0;
	}

	const char *pToken = pProperty->pValue;

	for (unsigned i = 0; i <= 3; i++)
	{
		char *pEnd;
		assert (pToken != 0);
		unsigned long nNumber = strtoul (pToken, &pEnd, 10);

		if (i < 3)
		{
			if (    pEnd == 0
			    || *pEnd != '.')
			{
				return 0;
			}
		}
		else
		{
			if (    pEnd != 0
			    && *pEnd != '\0')
			{
				return 0;
			}
		}

		if (nNumber > 255)
		{
			return 0;
		}

		m_IPAddress[i] = (u8) nNumber;

		assert (pEnd != 0);
		pToken = pEnd + 1;
	}

	return m_IPAddress;
}

void CProperties::SetString (const char *pPropertyName, const char *pValue)
{
	if (pValue == 0)
	{
		pValue = "";
	}

	assert (pPropertyName != 0);
	TPropertyPair *pProperty = Lookup (pPropertyName);
	if (pProperty == 0)
	{
		AddProperty (pPropertyName, pValue);
	}
	else
	{
		delete pProperty->pValue;

		pProperty->pValue = new char[strlen (pValue)+1];
		assert (pProperty->pValue != 0);
		strcpy (pProperty->pValue, pValue);
	}
}

void CProperties::SetNumber (const char *pPropertyName, unsigned nValue, unsigned nBase)
{
	CString Value;
	assert (nBase == 10 || nBase == 16);
	Value.Format (nBase == 10 ? "%u" : "0x%X", nValue);

	assert (pPropertyName != 0);
	SetString (pPropertyName, Value);
}

void CProperties::SetSignedNumber (const char *pPropertyName, int nValue)
{
	CString Value;
	Value.Format ("%d", nValue);

	assert (pPropertyName != 0);
	SetString (pPropertyName, Value);
}

void CProperties::SetIPAddress (const char *pPropertyName, const u8 *pAddress)
{
	CString Value;
	assert (pAddress != 0);
	Value.Format ("%u.%u.%u.%u", (unsigned) pAddress[0], (unsigned) pAddress[1],
				     (unsigned) pAddress[2], (unsigned) pAddress[3]);

	assert (pPropertyName != 0);
	SetString (pPropertyName, Value);
}

void CProperties::RemoveAll (void)
{
	TPtrListElement *pElement;
	while ((pElement  = m_SectionList.GetFirst ()) !=  0)
	{
		TSection *pSection = (TSection *) m_SectionList.GetPtr (pElement);
		assert (pSection != 0);
		CPtrArray *pPropArray = &pSection->PropArray;

		for (unsigned i = 0; i < pPropArray->GetCount (); i++)
		{
			TPropertyPair *pProperty = (TPropertyPair *) (*pPropArray)[i];
			assert (pProperty != 0);

			delete [] (char *) pProperty->pName;
			delete [] (char *) pProperty->pValue;
			delete pProperty;
		}

		while (pPropArray->GetCount () > 0)
		{
			pPropArray->RemoveLast ();
		}

		delete pSection;

		m_SectionList.Remove (pElement);
	}

	m_CurrentSectionName = DefaultSection;
	m_pCurrentSection = nullptr;
}

void CProperties::AddProperty (const char*pPropertyName, const char *pValue)
{
	if (IsSet (pPropertyName))
	{
		return;
	}

	TPropertyPair *pProperty = new TPropertyPair;
	assert (pProperty != 0);

	assert (pPropertyName != 0);
	size_t nNameLen = strlen (pPropertyName);
	assert (nNameLen > 0);
	pProperty->pName = new char[nNameLen+1];
	assert (pProperty->pName != 0);
	strcpy (pProperty->pName, pPropertyName);

	assert (pValue != 0);
	pProperty->pValue = new char[strlen (pValue)+1];
	assert (pProperty->pValue != 0);
	strcpy (pProperty->pValue, pValue);

	if (m_pCurrentSection == 0)
	{
		m_pCurrentSection = new TSection;
		assert (m_pCurrentSection != 0);

		m_pCurrentSection->Name = m_CurrentSectionName;

		TPtrListElement *pElement = m_SectionList.GetFirst ();
		while (pElement != 0)
		{
			TPtrListElement *pNext = m_SectionList.GetNext (pElement);
			if (pNext == 0)
			{
				break;
			}

			pElement = pNext;
		}

		m_SectionList.InsertAfter (pElement, m_pCurrentSection);
	}

	m_pCurrentSection->PropArray.Append (pProperty);
}

boolean CProperties::GetFirst (void)
{
	m_pGetSection = m_SectionList.GetFirst ();
	m_nGetIndex = 0;

	if (m_pGetSection == 0)
	{
		m_CurrentSectionName = DefaultSection;

		return FALSE;
	}

	TSection *pSection = (TSection *) m_SectionList.GetPtr (m_pGetSection);
	assert (pSection != 0);

	m_CurrentSectionName = pSection->Name;

	return pSection->PropArray.GetCount () > m_nGetIndex;
}

boolean CProperties::GetNext (void)
{
	m_nGetIndex++;

	assert (m_pGetSection != 0);
	TSection *pSection = (TSection *) m_SectionList.GetPtr (m_pGetSection);
	assert (pSection != 0);

	if (pSection->PropArray.GetCount () > m_nGetIndex)
	{
		return TRUE;
	}

	m_pGetSection = m_SectionList.GetNext (m_pGetSection);
	if (m_pGetSection == 0)
	{
		m_CurrentSectionName = DefaultSection;
		m_pCurrentSection = LookupSection (m_CurrentSectionName);

		return FALSE;
	}

	pSection = (TSection *) m_SectionList.GetPtr (m_pGetSection);
	assert (pSection != 0);

	m_CurrentSectionName = pSection->Name;
	m_pCurrentSection = pSection;

	m_nGetIndex = 0;

	return TRUE;
}

const char *CProperties::GetSectionName (void) const
{
	return m_CurrentSectionName;
}

const char *CProperties::GetName (void) const
{
	assert (m_pGetSection != 0);
	TSection *pSection = (TSection *) m_SectionList.GetPtr (m_pGetSection);
	assert (pSection != 0);

	assert (m_nGetIndex < pSection->PropArray.GetCount ());
	TPropertyPair *pProperty = (TPropertyPair *) pSection->PropArray[m_nGetIndex];
	assert (pProperty != 0);
	assert (pProperty->pName != 0);

	return pProperty->pName;
}

const char *CProperties::GetValue (void) const
{
	assert (m_pGetSection != 0);
	TSection *pSection = (TSection *) m_SectionList.GetPtr (m_pGetSection);
	assert (pSection != 0);

	assert (m_nGetIndex < pSection->PropArray.GetCount ());
	TPropertyPair *pProperty = (TPropertyPair *) pSection->PropArray[m_nGetIndex];
	assert (pProperty != 0);
	assert (pProperty->pValue != 0);

	return pProperty->pValue;
}

TSection *CProperties::LookupSection (const char*pSectionName) const
{
	assert (pSectionName != 0);

	TPtrListElement *pElement = m_SectionList.GetFirst ();
	while (pElement != 0)
	{
		TSection *pSection = (TSection *) m_SectionList.GetPtr (pElement);
		assert (pSection != 0);

		if (pSection->Name.Compare (pSectionName) == 0)
		{
			return pSection;
		}

		pElement = m_SectionList.GetNext (pElement);
	}

	return 0;
}


TPropertyPair *CProperties::Lookup (const char*pPropertyName) const
{
	if (m_pCurrentSection == 0)
	{
		return 0;
	}

	for (unsigned i = 0; i < m_pCurrentSection->PropArray.GetCount (); i++)
	{
		TPropertyPair *pProperty = (TPropertyPair *) m_pCurrentSection->PropArray[i];
		assert (pProperty != 0);
		if (strcmp (pProperty->pName, pPropertyName) == 0)
		{
			return pProperty;
		}
	}

	return 0;
}

#ifndef NDEBUG

void CProperties::Dump (const char *pSource) const
{
	CLogger::Get ()->Write (pSource, LogDebug, "Dumping properties:");

	TPtrListElement *pElement = m_SectionList.GetFirst ();
	while (pElement != 0)
	{
		TSection *pSection = (TSection *) m_SectionList.GetPtr (pElement);
		assert (pSection != 0);

		if (pSection->Name.Compare (DefaultSection) != 0)
		{
			CLogger::Get ()->Write (pSource, LogDebug, "[%s]",
						static_cast<const char *> (pSection->Name));
		}

		for (unsigned i = 0; i < pSection->PropArray.GetCount (); i++)
		{
			TPropertyPair *pProperty = (TPropertyPair *) pSection->PropArray[i];
			assert (pProperty != 0);

			CLogger::Get ()->Write (pSource, LogDebug, "%s=%s",
						pProperty->pName, pProperty->pValue);
		}

		pElement = m_SectionList.GetNext (pElement);
	}
}

#endif
