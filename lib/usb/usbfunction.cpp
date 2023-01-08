//
// usbfunction.cpp
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
#include <circle/usb/usbfunction.h>
#include <circle/usb/usbdevice.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/usb/usbendpoint.h>
#include <circle/logger.h>
#include <assert.h>

static const char FromUSBFunction[] = "usbfct";

CUSBFunction::CUSBFunction (CUSBDevice *pDevice, CUSBConfigurationParser *pConfigParser)
:	m_pDevice (pDevice),
	m_pConfigParser (0),
	m_pInterfaceDesc (0)
{
	assert (m_pDevice != 0);

	assert (pConfigParser != 0);
	m_pConfigParser = new CUSBConfigurationParser (pConfigParser);
	assert (m_pConfigParser != 0);

	m_pInterfaceDesc = (TUSBInterfaceDescriptor *) m_pConfigParser->GetCurrentDescriptor ();
	assert (m_pInterfaceDesc != 0);
}

CUSBFunction::CUSBFunction (CUSBFunction *pFunction)
:	m_pDevice (0),
	m_pConfigParser (0),
	m_pInterfaceDesc (0)
{
	assert (pFunction != 0);
	m_pDevice = pFunction->m_pDevice;
	assert (m_pDevice != 0);

	assert (pFunction->m_pConfigParser != 0);
	m_pConfigParser = new CUSBConfigurationParser (pFunction->m_pConfigParser);
	assert (m_pConfigParser != 0);

	m_pInterfaceDesc = (TUSBInterfaceDescriptor *) m_pConfigParser->GetCurrentDescriptor ();
	assert (m_pInterfaceDesc != 0);
}

CUSBFunction::~CUSBFunction (void)
{
	m_pInterfaceDesc = 0;

	delete m_pConfigParser;
	m_pConfigParser = 0;

	m_pDevice = 0;
}

boolean CUSBFunction::Initialize (void)
{
	return TRUE;
}

boolean CUSBFunction::Configure (void)
{
	assert (m_pInterfaceDesc != 0);
	if (m_pInterfaceDesc->bAlternateSetting != 0)
	{
		if (GetHost ()->ControlMessage (GetEndpoint0 (),
						REQUEST_OUT | REQUEST_TO_INTERFACE, SET_INTERFACE,
						m_pInterfaceDesc->bAlternateSetting,
						m_pInterfaceDesc->bInterfaceNumber, 0, 0) < 0)
		{
			CLogger::Get ()->Write (FromUSBFunction, LogError, "Cannot set interface");

			return FALSE;
		}
	}

	return TRUE;
}

boolean CUSBFunction::ReScanDevices (void)
{
	return FALSE;
}

boolean CUSBFunction::RemoveDevice (void)
{
	assert (m_pDevice != 0);
	return m_pDevice->RemoveDevice ();
}

CString *CUSBFunction::GetInterfaceName () const
{
	CString *pString = new CString ("unknown");
	assert (pString != 0);

	if (   m_pInterfaceDesc != 0
	    && m_pInterfaceDesc->bInterfaceClass != 0x00
	    && m_pInterfaceDesc->bInterfaceClass != 0xFF)
	{
		pString->Format ("int%x-%x-%x",
				 (unsigned) m_pInterfaceDesc->bInterfaceClass,
				 (unsigned) m_pInterfaceDesc->bInterfaceSubClass,
				 (unsigned) m_pInterfaceDesc->bInterfaceProtocol);
	}

	return pString;
}

u8 CUSBFunction::GetNumEndpoints (void) const
{
	assert (m_pInterfaceDesc != 0);
	return m_pInterfaceDesc->bNumEndpoints;
}

CUSBDevice *CUSBFunction::GetDevice (void) const
{
	assert (m_pDevice != 0);
	return m_pDevice;
}

CUSBEndpoint *CUSBFunction::GetEndpoint0 (void) const
{
	assert (m_pDevice != 0);
	return m_pDevice->GetEndpoint0 ();
}

CUSBHostController *CUSBFunction::GetHost (void) const
{
	assert (m_pDevice != 0);
	return m_pDevice->GetHost ();
}

const TUSBDescriptor *CUSBFunction::GetDescriptor (u8 ucType)
{
	assert (m_pConfigParser != 0);
	return m_pConfigParser->GetDescriptor (ucType);
}

void CUSBFunction::ConfigurationError (const char *pSource) const
{
	assert (m_pConfigParser != 0);
	assert (pSource != 0);
	m_pConfigParser->Error (pSource);
}

boolean CUSBFunction::SelectInterfaceByClass (u8 uchClass, u8 uchSubClass, u8 uchProtocol)
{
	assert (m_pInterfaceDesc != 0);
	assert (m_pConfigParser != 0);
	assert (m_pDevice != 0);

	do
	{
		if (   m_pInterfaceDesc->bInterfaceClass    == uchClass
		    && m_pInterfaceDesc->bInterfaceSubClass == uchSubClass
		    && m_pInterfaceDesc->bInterfaceProtocol == uchProtocol)
		{
			return TRUE;
		}

		// skip to next interface in interface enumeration in class CDevice
		m_pDevice->GetDescriptor (DESCRIPTOR_INTERFACE);
	}
	while ((m_pInterfaceDesc = (TUSBInterfaceDescriptor *)
				m_pConfigParser->GetDescriptor (DESCRIPTOR_INTERFACE)) != 0);

	return FALSE;
}

u8 CUSBFunction::GetInterfaceNumber (void) const
{
	assert (m_pInterfaceDesc != 0);
	return m_pInterfaceDesc->bInterfaceNumber;
}

u8 CUSBFunction::GetInterfaceClass (void) const
{
	assert (m_pInterfaceDesc != 0);
	return m_pInterfaceDesc->bInterfaceClass;
}

u8 CUSBFunction::GetInterfaceSubClass (void) const
{
	assert (m_pInterfaceDesc != 0);
	return m_pInterfaceDesc->bInterfaceSubClass;
}

u8 CUSBFunction::GetInterfaceProtocol (void) const
{
	assert (m_pInterfaceDesc != 0);
	return m_pInterfaceDesc->bInterfaceProtocol;
}

const TUSBInterfaceDescriptor *CUSBFunction::GetInterfaceDescriptor (void) const
{
	assert (m_pInterfaceDesc != 0);
	return m_pInterfaceDesc;
}
