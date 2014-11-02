//
// usbdevice.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/usbdevice.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/usb/usbendpoint.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <circle/debug.h>
#include <assert.h>

#define MAX_CONFIG_DESC_SIZE		512		// best guess

struct TConfigurationHeader
{
	TUSBConfigurationDescriptor Configuration;
	TUSBInterfaceDescriptor	    Interface;
};

static const char FromDevice[] = "usbdev";

u8 CUSBDevice::s_ucNextAddress = USB_FIRST_DEDICATED_ADDRESS;

CUSBDevice::CUSBDevice (CUSBHostController *pHost, TUSBSpeed Speed, u8 ucHubAddress, u8 ucHubPortNumber)
:	m_pHost (pHost),
	m_ucAddress (USB_DEFAULT_ADDRESS),
	m_Speed (Speed),
	m_pEndpoint0 (0),
	m_ucHubAddress (ucHubAddress),
	m_ucHubPortNumber (ucHubPortNumber),
	m_pDeviceDesc (0),
	m_pConfigDesc (0),
	m_pConfigParser (0)
{
	assert (m_pHost != 0);

	assert (m_pEndpoint0 == 0);
	m_pEndpoint0 = new CUSBEndpoint (this);
	assert (m_pEndpoint0 != 0);
	
	assert (ucHubPortNumber >= 1);
}

CUSBDevice::CUSBDevice (CUSBDevice *pDevice)
:	m_pEndpoint0 (0),
	m_pDeviceDesc (0),
	m_pConfigDesc (0),
	m_pConfigParser (0)
{
	assert (pDevice != 0);

	m_pHost		  = pDevice->m_pHost;
	m_ucAddress	  = pDevice->m_ucAddress;
	m_Speed		  = pDevice->m_Speed;
	m_ucHubAddress	  = pDevice->m_ucHubAddress;
	m_ucHubPortNumber = pDevice->m_ucHubPortNumber;
	
	m_pEndpoint0 = new CUSBEndpoint (pDevice->m_pEndpoint0, this);
	assert (m_pEndpoint0 != 0);

	if (pDevice->m_pDeviceDesc != 0)
	{
		m_pDeviceDesc = new TUSBDeviceDescriptor;
		assert (m_pDeviceDesc != 0);

		memcpy (m_pDeviceDesc, pDevice->m_pDeviceDesc, sizeof (TUSBDeviceDescriptor));
	}

	if (pDevice->m_pConfigDesc != 0)
	{
		unsigned nTotalLength = pDevice->m_pConfigDesc->wTotalLength;
		assert (nTotalLength <= MAX_CONFIG_DESC_SIZE);

		m_pConfigDesc = (TUSBConfigurationDescriptor *) new u8[nTotalLength];
		assert (m_pConfigDesc != 0);

		memcpy (m_pConfigDesc, pDevice->m_pConfigDesc, nTotalLength);

		if (pDevice->m_pConfigParser != 0)
		{
			m_pConfigParser = new CUSBConfigurationParser (m_pConfigDesc, nTotalLength);
			assert (m_pConfigParser != 0);
		}
	}
}

CUSBDevice::~CUSBDevice (void)
{
	delete m_pConfigParser;
	m_pConfigParser = 0;

	delete m_pConfigDesc;
	m_pConfigDesc = 0;
	
	delete m_pDeviceDesc;
	m_pDeviceDesc = 0;
	
	delete m_pEndpoint0;
	m_pEndpoint0 = 0;
	
	m_pHost = 0;
}

boolean CUSBDevice::Initialize (void)
{
	assert (m_pDeviceDesc == 0);
	m_pDeviceDesc = new TUSBDeviceDescriptor;
	assert (m_pDeviceDesc != 0);
	
	assert (m_pHost != 0);
	assert (m_pEndpoint0 != 0);
	
	assert (sizeof *m_pDeviceDesc >= USB_DEFAULT_MAX_PACKET_SIZE);
	if (m_pHost->GetDescriptor (m_pEndpoint0,
				    DESCRIPTOR_DEVICE, DESCRIPTOR_INDEX_DEFAULT,
				    m_pDeviceDesc, USB_DEFAULT_MAX_PACKET_SIZE)
	    != USB_DEFAULT_MAX_PACKET_SIZE)
	{
		CLogger::Get ()->Write (FromDevice, LogError, "Cannot get device descriptor (short)");

		delete m_pDeviceDesc;
		m_pDeviceDesc = 0;

		return FALSE;
	}

	if (   m_pDeviceDesc->bLength	      != sizeof *m_pDeviceDesc
	    || m_pDeviceDesc->bDescriptorType != DESCRIPTOR_DEVICE)
	{
		CLogger::Get ()->Write (FromDevice, LogError, "Invalid device descriptor");

		delete m_pDeviceDesc;
		m_pDeviceDesc = 0;

		return FALSE;
	}

	m_pEndpoint0->SetMaxPacketSize (m_pDeviceDesc->bMaxPacketSize0);

	if (m_pHost->GetDescriptor (m_pEndpoint0,
				    DESCRIPTOR_DEVICE, DESCRIPTOR_INDEX_DEFAULT,
				    m_pDeviceDesc, sizeof *m_pDeviceDesc)
	    != (int) sizeof *m_pDeviceDesc)
	{
		CLogger::Get ()->Write (FromDevice, LogError, "Cannot get device descriptor");

		delete m_pDeviceDesc;
		m_pDeviceDesc = 0;

		return FALSE;
	}

#ifndef NDEBUG
	//debug_hexdump (m_pDeviceDesc, sizeof *m_pDeviceDesc, FromDevice);
#endif
	
	u8 ucAddress = s_ucNextAddress++;
	if (ucAddress > USB_MAX_ADDRESS)
	{
		CLogger::Get ()->Write (FromDevice, LogError, "Too many devices");

		return FALSE;
	}

	if (!m_pHost->SetAddress (m_pEndpoint0, ucAddress))
	{
		CLogger::Get ()->Write (FromDevice, LogError,
					"Cannot set address %u", (unsigned) ucAddress);

		return FALSE;
	}
	
	SetAddress (ucAddress);

	assert (m_pConfigDesc == 0);
	m_pConfigDesc = new TUSBConfigurationDescriptor;
	assert (m_pConfigDesc != 0);

	if (m_pHost->GetDescriptor (m_pEndpoint0,
				    DESCRIPTOR_CONFIGURATION, DESCRIPTOR_INDEX_DEFAULT,
				    m_pConfigDesc, sizeof *m_pConfigDesc)
	    != (int) sizeof *m_pConfigDesc)
	{
		CLogger::Get ()->Write (FromDevice, LogError,
					"Cannot get configuration descriptor (short)");

		delete m_pConfigDesc;
		m_pConfigDesc = 0;

		return FALSE;
	}

	if (   m_pConfigDesc->bLength         != sizeof *m_pConfigDesc
	    || m_pConfigDesc->bDescriptorType != DESCRIPTOR_CONFIGURATION
	    || m_pConfigDesc->wTotalLength    >  MAX_CONFIG_DESC_SIZE)
	{
		CLogger::Get ()->Write (FromDevice, LogError, "Invalid configuration descriptor");
		
		delete m_pConfigDesc;
		m_pConfigDesc = 0;

		return FALSE;
	}

	unsigned nTotalLength = m_pConfigDesc->wTotalLength;

	delete m_pConfigDesc;

	m_pConfigDesc = (TUSBConfigurationDescriptor *) new u8[nTotalLength];
	assert (m_pConfigDesc != 0);

	if (m_pHost->GetDescriptor (m_pEndpoint0,
				    DESCRIPTOR_CONFIGURATION, DESCRIPTOR_INDEX_DEFAULT,
				    m_pConfigDesc, nTotalLength)
	    != (int) nTotalLength)
	{
		CLogger::Get ()->Write (FromDevice, LogError, "Cannot get configuration descriptor");

		delete m_pConfigDesc;
		m_pConfigDesc = 0;

		return FALSE;
	}

#ifndef NDEBUG
	//debug_hexdump (m_pConfigDesc, nTotalLength, FromDevice);
#endif

	assert (m_pConfigParser == 0);
	m_pConfigParser = new CUSBConfigurationParser (m_pConfigDesc, nTotalLength);
	assert (m_pConfigParser != 0);

	if (!m_pConfigParser->IsValid ())
	{
		ConfigurationError (FromDevice);

		return FALSE;
	}

	return TRUE;
}

boolean CUSBDevice::Configure (void)
{
	assert (m_pHost != 0);
	assert (m_pEndpoint0 != 0);

	if (m_pConfigDesc == 0)		// not initialized
	{
		return FALSE;
	}

	if (!m_pHost->SetConfiguration (m_pEndpoint0, m_pConfigDesc->bConfigurationValue))
	{
		CLogger::Get ()->Write (FromDevice, LogError, "Cannot set configuration (%u)",
					(unsigned) m_pConfigDesc->bConfigurationValue);

		return FALSE;
	}

	return TRUE;
}

CString *CUSBDevice::GetName (TDeviceNameSelector Selector) const
{
	CString *pString = new CString;
	assert (pString != 0);
	
	switch (Selector)
	{
	case DeviceNameVendor:
		assert (m_pDeviceDesc != 0);
		pString->Format ("ven%x-%x",
				 (unsigned) m_pDeviceDesc->idVendor,
				 (unsigned) m_pDeviceDesc->idProduct);
		break;
		
	case DeviceNameDevice:
		assert (m_pDeviceDesc != 0);
		if (   m_pDeviceDesc->bDeviceClass == 0
		    || m_pDeviceDesc->bDeviceClass == 0xFF)
		{
			goto unknown;
		}
		pString->Format ("dev%x-%x-%x",
				 (unsigned) m_pDeviceDesc->bDeviceClass,
				 (unsigned) m_pDeviceDesc->bDeviceSubClass,
				 (unsigned) m_pDeviceDesc->bDeviceProtocol);
		break;
		
	case DeviceNameInterface: {
		TConfigurationHeader *pConfig = (TConfigurationHeader *) m_pConfigDesc;
		assert (pConfig != 0);
		if (   pConfig->Configuration.wTotalLength < sizeof *pConfig
		    || pConfig->Interface.bInterfaceClass == 0
		    || pConfig->Interface.bInterfaceClass == 0xFF)
		{
			goto unknown;
		}
		pString->Format ("int%x-%x-%x",
				 (unsigned) pConfig->Interface.bInterfaceClass,
				 (unsigned) pConfig->Interface.bInterfaceSubClass,
				 (unsigned) pConfig->Interface.bInterfaceProtocol);
		} break;

	default:
		assert (0);
	unknown:
		*pString = "unknown";
		break;
	}
	
	return pString;
}

void CUSBDevice::SetAddress (u8 ucAddress)
{
	assert (ucAddress <= USB_MAX_ADDRESS);
	m_ucAddress = ucAddress;

	CLogger::Get ()->Write (FromDevice, LogDebug, "Device address set to %u", (unsigned) m_ucAddress);
}

u8 CUSBDevice::GetAddress (void) const
{
	return m_ucAddress;
}

TUSBSpeed CUSBDevice::GetSpeed (void) const
{
	return m_Speed;
}

u8 CUSBDevice::GetHubAddress (void) const
{
	return m_ucHubAddress;
}

u8 CUSBDevice::GetHubPortNumber (void) const
{
	return m_ucHubPortNumber;
}

CUSBEndpoint *CUSBDevice::GetEndpoint0 (void) const
{
	assert (m_pEndpoint0 != 0);
	return m_pEndpoint0;
}

CUSBHostController *CUSBDevice::GetHost (void) const
{
	assert (m_pHost != 0);
	return m_pHost;
}

const TUSBDeviceDescriptor *CUSBDevice::GetDeviceDescriptor (void) const
{
	assert (m_pDeviceDesc != 0);
	return m_pDeviceDesc;
}

const TUSBConfigurationDescriptor *CUSBDevice::GetConfigurationDescriptor (void) const
{
	assert (m_pConfigDesc != 0);
	return m_pConfigDesc;
}

const TUSBDescriptor *CUSBDevice::GetDescriptor (u8 ucType)
{
	assert (m_pConfigParser != 0);
	return m_pConfigParser->GetDescriptor (ucType);
}

void CUSBDevice::ConfigurationError (const char *pSource) const
{
	assert (m_pConfigParser != 0);
	m_pConfigParser->Error (pSource);
}
