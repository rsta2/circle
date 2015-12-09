//
// usbendpoint.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2015  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/usbendpoint.h>
#include <assert.h>

CUSBEndpoint::CUSBEndpoint (CUSBDevice *pDevice)
:	m_pDevice (pDevice),
	m_ucNumber (0),
	m_Type (EndpointTypeControl),
	m_bDirectionIn (FALSE),
	m_nMaxPacketSize (USB_DEFAULT_MAX_PACKET_SIZE),
	m_nInterval (1),
	m_NextPID (USBPIDSetup)
{
	assert (m_pDevice != 0);
}

CUSBEndpoint::CUSBEndpoint (CUSBDevice *pDevice, const TUSBEndpointDescriptor *pDesc)
:	m_pDevice (pDevice),
	m_nInterval (1)
{
	assert (m_pDevice != 0);

	assert (pDesc != 0);
	assert (pDesc->bLength == sizeof *pDesc);
	assert (pDesc->bDescriptorType == DESCRIPTOR_ENDPOINT);

	switch (pDesc->bmAttributes & 0x03)
	{
	case 2:
		m_Type = EndpointTypeBulk;
		m_NextPID = USBPIDData0;
		break;

	case 3:
		m_Type = EndpointTypeInterrupt;
		m_NextPID = USBPIDData0;
		break;

	default:
		assert (0);	// endpoint configuration should be checked by function driver
		return;
	}
	
	m_ucNumber       = pDesc->bEndpointAddress & 0x0F;
	m_bDirectionIn   = pDesc->bEndpointAddress & 0x80 ? TRUE : FALSE;
	m_nMaxPacketSize = pDesc->wMaxPacketSize;
	
	if (m_Type == EndpointTypeInterrupt)
	{
		u8 ucInterval = pDesc->bInterval;
		if (ucInterval < 1)
		{
			ucInterval = 1;
		}

		// see USB 2.0 spec chapter 9.6.6
		if (m_pDevice->GetSpeed () != USBSpeedHigh)
		{
			m_nInterval = ucInterval;
		}
		else
		{
			if (ucInterval > 16)
			{
				ucInterval = 16;
			}

			unsigned nValue = 1 << (ucInterval - 1);

			m_nInterval = nValue / 8;

			if (m_nInterval < 1)
			{
				m_nInterval = 1;
			}
		}

		// interval 20ms is minimum to reduce interrupt rate
		if (m_nInterval < 20)
		{
			m_nInterval = 20;
		}
	}
}

CUSBEndpoint::CUSBEndpoint (CUSBEndpoint *pEndpoint, CUSBDevice *pDevice)
{
	assert (pEndpoint != 0);

	m_pDevice = pDevice;
	assert (m_pDevice != 0);

	m_ucNumber	 = pEndpoint->m_ucNumber;
	m_Type		 = pEndpoint->m_Type;
	m_bDirectionIn	 = pEndpoint->m_bDirectionIn;
	m_nMaxPacketSize = pEndpoint->m_nMaxPacketSize;
	m_nInterval      = pEndpoint->m_nInterval;
	m_NextPID	 = pEndpoint->m_NextPID;
}

CUSBEndpoint::~CUSBEndpoint (void)
{
	m_pDevice = 0;
}

CUSBDevice *CUSBEndpoint::GetDevice (void) const
{
	assert (m_pDevice != 0);
	return m_pDevice;
}

u8 CUSBEndpoint::GetNumber (void) const
{
	return m_ucNumber;
}

TEndpointType CUSBEndpoint::GetType (void) const
{
	return m_Type;
}

boolean CUSBEndpoint::IsDirectionIn (void) const
{
	return m_bDirectionIn;
}

void CUSBEndpoint::SetMaxPacketSize (u32 nMaxPacketSize)
{
	m_nMaxPacketSize = nMaxPacketSize;
}

u32 CUSBEndpoint::GetMaxPacketSize (void) const
{
	return m_nMaxPacketSize;
}

unsigned CUSBEndpoint::GetInterval (void) const
{
	assert (m_Type == EndpointTypeInterrupt);

	return m_nInterval;
}

TUSBPID CUSBEndpoint::GetNextPID (boolean bStatusStage)
{
	if (bStatusStage)
	{
		assert (m_Type == EndpointTypeControl);

		return USBPIDData1;
	}
	
	return m_NextPID;
}

void CUSBEndpoint::SkipPID (unsigned nPackets, boolean bStatusStage)
{
	assert (   m_Type == EndpointTypeControl
		|| m_Type == EndpointTypeBulk
		|| m_Type == EndpointTypeInterrupt);
	
	if (!bStatusStage)
	{
		switch (m_NextPID)
		{
		case USBPIDSetup:
			m_NextPID = USBPIDData1;
			break;

		case USBPIDData0:
			if (nPackets & 1)
			{
				m_NextPID = USBPIDData1;
			}
			break;
			
		case USBPIDData1:
			if (nPackets & 1)
			{
				m_NextPID = USBPIDData0;
			}
			break;

		default:
			assert (0);
			break;
		}
	}
	else
	{
		assert (m_Type == EndpointTypeControl);

		m_NextPID = USBPIDSetup;
	}
}

void CUSBEndpoint::ResetPID (void)
{
	assert (m_Type == EndpointTypeBulk);

	m_NextPID = USBPIDData0;
}
