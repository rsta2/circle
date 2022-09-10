//
// usbstring.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2022  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/usbstring.h>
#include <circle/usb/usbdevice.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/util.h>
#include <assert.h>

#define USBSTR_MIN_LENGTH	4

#define USBSTR_DEFAULT_LANGID	0x409

CUSBString::CUSBString (CUSBDevice *pDevice)
:	m_pDevice (pDevice),
	m_pUSBString (0),
	m_pString (new CString)
{
	assert (m_pDevice != 0);
	assert (m_pString != 0);
}

CUSBString::CUSBString (CUSBString *pParent)
:	m_pUSBString (0),
	m_pString (new CString)
{
	assert (pParent != 0);
	m_pDevice = pParent->m_pDevice;

	if (pParent->m_pUSBString != 0)
	{
		m_pUSBString = (TUSBStringDescriptor *) new u8[pParent->m_pUSBString->bLength];
		assert (m_pUSBString != 0);
		memcpy (m_pUSBString, pParent->m_pUSBString, pParent->m_pUSBString->bLength);
	}

	assert (m_pString != 0);
	assert (pParent->m_pString != 0);
	m_pString = pParent->m_pString;
}

CUSBString::~CUSBString (void)
{
	delete m_pString;
	m_pString = 0;

	delete [] m_pUSBString;
	m_pUSBString = 0;

	m_pDevice = 0;
}

boolean CUSBString::GetFromDescriptor (u8 ucID, u16 usLanguageID)
{
	assert (ucID > 0);

	delete [] m_pUSBString;
	m_pUSBString = (TUSBStringDescriptor *) new u8[USBSTR_MIN_LENGTH];
	assert (m_pUSBString != 0);

	assert (m_pDevice != 0);
	if (m_pDevice->GetHost ()->GetDescriptor (m_pDevice->GetEndpoint0 (),
						  DESCRIPTOR_STRING, ucID,
						  m_pUSBString, USBSTR_MIN_LENGTH,
						  REQUEST_IN, usLanguageID) < 0)
	{
		return FALSE;
	}

	u8 ucLength = m_pUSBString->bLength;
	if (   ucLength < 2
	    || (ucLength & 1) != 0
	    || m_pUSBString->bDescriptorType != DESCRIPTOR_STRING)
	{
		return FALSE;
	}

	if (ucLength > USBSTR_MIN_LENGTH)
	{
		delete m_pUSBString;
		m_pUSBString = (TUSBStringDescriptor *) new u8[ucLength];
		assert (m_pUSBString != 0);

		if (m_pDevice->GetHost ()->GetDescriptor (m_pDevice->GetEndpoint0 (),
							  DESCRIPTOR_STRING, ucID,
							  m_pUSBString, ucLength,
							  REQUEST_IN, usLanguageID) != (int) ucLength)
		{
			return FALSE;
		}

		if (   m_pUSBString->bLength != ucLength
		    || (m_pUSBString->bLength & 1) != 0
		    || m_pUSBString->bDescriptorType != DESCRIPTOR_STRING)
		{
			return FALSE;
		}
	}

	// convert into ASCII string
	assert (m_pUSBString->bLength > 2);
	assert ((m_pUSBString->bLength & 1) == 0);
	size_t nLength = (m_pUSBString->bLength-2) / 2;

	assert (nLength <= (255-2) / 2);
	char Buffer[nLength+1];
	
	for (unsigned i = 0; i < nLength; i++)
	{
		u16 usChar = m_pUSBString->bString[i];
		if (   usChar != 0
		    && (   usChar < ' '
		        || usChar > '~'))
		{
			usChar = '_';
		}
		
		Buffer[i] = (char) usChar;
	}
	Buffer[nLength] = '\0';

	delete m_pString;
	m_pString = new CString (Buffer);
	assert (m_pString != 0);

	return TRUE;
}

const char *CUSBString::Get (void) const
{
	return *m_pString;
}

u16 CUSBString::GetLanguageID (void)
{
	TUSBStringDescriptor *pLanguageIDs = (TUSBStringDescriptor *) new u8[USBSTR_MIN_LENGTH];
	assert (pLanguageIDs != 0);

	assert (m_pDevice != 0);
	if (m_pDevice->GetHost ()->GetDescriptor (m_pDevice->GetEndpoint0 (),
						  DESCRIPTOR_STRING, 0,
						  pLanguageIDs, USBSTR_MIN_LENGTH) < 0)
	{
		delete [] pLanguageIDs;

		return USBSTR_DEFAULT_LANGID;
	}

	u8 ucLength = pLanguageIDs->bLength;
	if (   ucLength < 4
	    || (ucLength & 1) != 0
	    || pLanguageIDs->bDescriptorType != DESCRIPTOR_STRING)
	{
		delete [] pLanguageIDs;

		return USBSTR_DEFAULT_LANGID;
	}

	if (ucLength > USBSTR_MIN_LENGTH)
	{
		delete [] pLanguageIDs;
		pLanguageIDs = (TUSBStringDescriptor *) new u8[ucLength];
		assert (pLanguageIDs != 0);

		if (m_pDevice->GetHost ()->GetDescriptor (m_pDevice->GetEndpoint0 (),
							  DESCRIPTOR_STRING, 0,
							  pLanguageIDs, ucLength) != (int) ucLength)
		{
			delete [] pLanguageIDs;

			return USBSTR_DEFAULT_LANGID;
		}

		if (   pLanguageIDs->bLength != ucLength
		    || (pLanguageIDs->bLength & 1) != 0
		    || pLanguageIDs->bDescriptorType != DESCRIPTOR_STRING)
		{
			delete [] pLanguageIDs;

			return USBSTR_DEFAULT_LANGID;
		}
	}

	assert (pLanguageIDs->bLength >= 4);
	assert ((pLanguageIDs->bLength & 1) == 0);
	size_t nLength = (pLanguageIDs->bLength-2) / 2;

	// search for default language ID
	for (unsigned i = 0; i < nLength; i++)
	{
		if (pLanguageIDs->bString[i] == USBSTR_DEFAULT_LANGID)
		{
			delete [] pLanguageIDs;

			return USBSTR_DEFAULT_LANGID;
		}
	}

	// default language ID not found, use first ID
	u16 usResult = pLanguageIDs->bString[0];

	delete [] pLanguageIDs;

	return usResult;
}
