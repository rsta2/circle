//
// usbcdcethernet.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2019  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/usbcdcethernet.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/usb/usbstring.h>
#include <circle/usb/usb.h>
#include <circle/logger.h>
#include <circle/macros.h>
#include <assert.h>

struct TEthernetNetworkingFunctionalDescriptor
{
	u8	bLength;
	u8	bDescriptorType;
	u8	bDescriptorSubtype;
#define ETHERNET_NETWORKING_FUNCTIONAL_DESCRIPTOR	0x0F
	u8	iMACAddress;
	u32	bmEthernetStatistics;
	u16	wMaxSegmentSize;
	u16	wNumberMCFilters;
	u8	bNumberPowerFilters;
}
PACKED;

static const char FromCDCEthernet[] = "ucdceth";

CUSBCDCEthernetDevice::CUSBCDCEthernetDevice (CUSBFunction *pFunction)
:	CUSBFunction (pFunction),
	m_pEndpointBulkIn (0),
	m_pEndpointBulkOut (0)
{
}

CUSBCDCEthernetDevice::~CUSBCDCEthernetDevice (void)
{
	delete m_pEndpointBulkOut;
	m_pEndpointBulkOut = 0;

	delete m_pEndpointBulkIn;
	m_pEndpointBulkIn = 0;
}

boolean CUSBCDCEthernetDevice::Configure (void)
{
	// find Ethernet Networking Functional Descriptor
	const TEthernetNetworkingFunctionalDescriptor *pEthernetDesc;
	while ((pEthernetDesc = (TEthernetNetworkingFunctionalDescriptor *)
				GetDescriptor (DESCRIPTOR_CS_INTERFACE)) != 0)
	{
		if (pEthernetDesc->bDescriptorSubtype == ETHERNET_NETWORKING_FUNCTIONAL_DESCRIPTOR)
		{
			break;
		}
	}

	if (pEthernetDesc == 0)
	{
		ConfigurationError (FromCDCEthernet);

		return FALSE;
	}

	// find Data Class Interface
	const TUSBInterfaceDescriptor *pInterfaceDesc;
	while ((pInterfaceDesc = (TUSBInterfaceDescriptor *) GetDescriptor (DESCRIPTOR_INTERFACE)) != 0)
	{
		if (   pInterfaceDesc->bInterfaceClass    == 0x0A
		    && pInterfaceDesc->bInterfaceSubClass == 0x00
		    && pInterfaceDesc->bInterfaceProtocol == 0x00
		    && pInterfaceDesc->bNumEndpoints      >= 2)
		{
			break;
		}
	}

	if (pInterfaceDesc == 0)
	{
		ConfigurationError (FromCDCEthernet);

		return FALSE;
	}

	// init MAC address
	if (!InitMACAddress (pEthernetDesc->iMACAddress))
	{
		CLogger::Get ()->Write (FromCDCEthernet, LogError, "Cannot get MAC address");

		return FALSE;
	}

	CString MACString;
	m_MACAddress.Format (&MACString);
	CLogger::Get ()->Write (FromCDCEthernet, LogDebug, "MAC address is %s", (const char *) MACString);

	// get endpoints
	const TUSBEndpointDescriptor *pEndpointDesc;
	while ((pEndpointDesc = (TUSBEndpointDescriptor *) GetDescriptor (DESCRIPTOR_ENDPOINT)) != 0)
	{
		if ((pEndpointDesc->bmAttributes & 0x3F) == 0x02)		// Bulk
		{
			if ((pEndpointDesc->bEndpointAddress & 0x80) == 0x80)	// Input
			{
				if (m_pEndpointBulkIn != 0)
				{
					ConfigurationError (FromCDCEthernet);

					return FALSE;
				}

				m_pEndpointBulkIn = new CUSBEndpoint (GetDevice (), pEndpointDesc);
			}
			else							// Output
			{
				if (m_pEndpointBulkOut != 0)
				{
					ConfigurationError (FromCDCEthernet);

					return FALSE;
				}

				m_pEndpointBulkOut = new CUSBEndpoint (GetDevice (), pEndpointDesc);
			}
		}
	}

	if (   m_pEndpointBulkIn  == 0
	    || m_pEndpointBulkOut == 0)
	{
		ConfigurationError (FromCDCEthernet);

		return FALSE;
	}

	if (!CUSBFunction::Configure ())
	{
		CLogger::Get ()->Write (FromCDCEthernet, LogError, "Cannot set interface");

		return FALSE;
	}

	AddNetDevice ();

	return TRUE;
}

const CMACAddress *CUSBCDCEthernetDevice::GetMACAddress (void) const
{
	return &m_MACAddress;
}

boolean CUSBCDCEthernetDevice::SendFrame (const void *pBuffer, unsigned nLength)
{
	assert (m_pEndpointBulkOut != 0);
	assert (pBuffer != 0);
	assert (nLength <= FRAME_BUFFER_SIZE);
	return GetHost ()->Transfer (m_pEndpointBulkOut, (void *) pBuffer, nLength) >= 0;
}

boolean CUSBCDCEthernetDevice::ReceiveFrame (void *pBuffer, unsigned *pResultLength)
{
	assert (m_pEndpointBulkIn != 0);
	assert (pBuffer != 0);
	CUSBRequest URB (m_pEndpointBulkIn, pBuffer, FRAME_BUFFER_SIZE);

	URB.SetCompleteOnNAK ();

	if (!GetHost ()->SubmitBlockingRequest (&URB))
	{
		return FALSE;
	}

	u32 nResultLength = URB.GetResultLength ();
	if (nResultLength == 0)
	{
		return FALSE;
	}

	assert (pResultLength != 0);
	*pResultLength = nResultLength;

	return TRUE;
}

boolean CUSBCDCEthernetDevice::InitMACAddress (u8 iMACAddress)
{
	CUSBString MACAddressString (GetDevice ());
	if (   iMACAddress == 0
	    || !MACAddressString.GetFromDescriptor (iMACAddress, MACAddressString.GetLanguageID ()))
	{
		return FALSE;
	}

	const char *pMACString = MACAddressString.Get ();
	assert (pMACString != 0);

	u8 MACAddress[MAC_ADDRESS_SIZE];
	for (unsigned i = 0; i < MAC_ADDRESS_SIZE; i++)
	{
		u8 ucByte = 0;
		for (unsigned j = 0; j < 2; j++)
		{
			char chChar = *pMACString++;
			if (chChar > '9')
			{
				chChar -= 'A'-'9'-1;
			}
			chChar -= '0';

			if (!('\0' <= chChar && chChar <= '\xF'))
			{
				return FALSE;
			}

			ucByte <<= 4;
			ucByte |= (u8) chChar;
		}

		MACAddress[i] = ucByte;
	}

	m_MACAddress.Set (MACAddress);

	return TRUE;
}
