//
// usbcdcethernet.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2025  R. Stange <rsta2@gmx.net>
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
#include <circle/util.h>
#include <assert.h>

#define SET_ETHERNET_PACKET_FILTER		0x43
	#define	PACKET_TYPE_ALL_MULTICAST	BIT (1)
	#define	PACKET_TYPE_DIRECTED		BIT (2)
	#define	PACKET_TYPE_BROADCAST		BIT (3)

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
	m_uchControlInterface (GetInterfaceNumber ()),
	m_iMACAddress (GetMACAddressStringIndex ()),
	m_bInterfaceOK (SelectInterfaceByClass (10, 0, 0, 2)),
	m_pEndpointBulkIn (0),
	m_pEndpointBulkOut (0),
	m_pURB (0),
	m_nRxLength (0)
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
	if (!m_bInterfaceOK)
	{
		ConfigurationError (FromCDCEthernet);

		return FALSE;
	}

	// init MAC address
	if (   !m_iMACAddress
	    || !InitMACAddress (m_iMACAddress))
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
	boolean bResult = FALSE;

	if (m_nRxLength != 0)
	{
		assert (pBuffer != 0);
		assert (m_nRxLength <= FRAME_BUFFER_SIZE);
		memcpy (pBuffer, m_RxBuffer, m_nRxLength);

		assert (pResultLength != 0);
		*pResultLength = m_nRxLength;

		m_nRxLength = 0;

		bResult = TRUE;
	}

	if (m_pURB == 0)
	{
		assert (m_pEndpointBulkIn != 0);
		m_pURB = new CUSBRequest (m_pEndpointBulkIn, m_RxBuffer, FRAME_BUFFER_SIZE);
		assert (m_pURB != 0);

		m_pURB->SetCompletionRoutine (CompletionRoutine, 0, this);

		m_pURB->SetCompleteOnNAK ();

		GetHost ()->SubmitAsyncRequest (m_pURB);
	}

	return bResult;
}

void CUSBCDCEthernetDevice::CompletionRoutine (CUSBRequest *pURB, void *pParam, void *pContext)
{
	CUSBCDCEthernetDevice *pThis = (CUSBCDCEthernetDevice *) pContext;
	assert (pThis != 0);

	assert (pThis->m_pURB == pURB);

	if (pURB->GetStatus () != 0)
	{
		assert (pThis->m_nRxLength == 0);
		pThis->m_nRxLength = pURB->GetResultLength ();
		assert (pThis->m_nRxLength <= FRAME_BUFFER_SIZE);
	}

	delete pURB;

	pThis->m_pURB = 0;
}

boolean CUSBCDCEthernetDevice::SetMulticastFilter (const u8 Groups[][MAC_ADDRESS_SIZE])
{
	u16 usFilter = PACKET_TYPE_DIRECTED | PACKET_TYPE_BROADCAST;

	// Enable all multicasts, if at least one host group is requested.
	if (Groups[0][0])
	{
		usFilter |= PACKET_TYPE_ALL_MULTICAST;
	}

	return GetHost ()->ControlMessage (GetEndpoint0 (),
					   REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_INTERFACE,
					   SET_ETHERNET_PACKET_FILTER,
					   usFilter,
					   m_uchControlInterface, 0, 0) >= 0;
}

u8 CUSBCDCEthernetDevice::GetMACAddressStringIndex (void)
{
	// find Ethernet Networking Functional Descriptor
	const TEthernetNetworkingFunctionalDescriptor *pEthernetDesc;
	while ((pEthernetDesc = (TEthernetNetworkingFunctionalDescriptor *)
				GetDescriptor (DESCRIPTOR_CS_INTERFACE)) != 0)
	{
		if (pEthernetDesc->bDescriptorSubtype == ETHERNET_NETWORKING_FUNCTIONAL_DESCRIPTOR)
		{
			assert (pEthernetDesc->iMACAddress != 0);
			return pEthernetDesc->iMACAddress;
		}
	}

	return 0;
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
