//
// usbendpoint.cpp
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
#include <circle/usb/usbendpoint.h>
#include <circle/sysconfig.h>
#include <circle/logger.h>
#include <assert.h>

CUSBEndpoint::CUSBEndpoint (CUSBDevice *pDevice)
:	m_pDevice (pDevice),
	m_ucNumber (0),
	m_Type (EndpointTypeControl),
	m_bDirectionIn (FALSE),
	m_nMaxPacketSize (USB_DEFAULT_MAX_PACKET_SIZE)
#if RASPPI <= 3
	, m_nInterval (1),
	m_NextPID (USBPIDSetup)
#endif
{
	assert (m_pDevice != 0);

#if RASPPI >= 4
	m_pXHCIEndpoint = new CXHCIEndpoint ((CXHCIUSBDevice *) m_pDevice,
					     (CXHCIDevice *) m_pDevice->GetHost ());
#endif
}

CUSBEndpoint::CUSBEndpoint (CUSBDevice *pDevice, const TUSBEndpointDescriptor *pDesc)
:	m_pDevice (pDevice)
#if RASPPI <= 3
	, m_nInterval (1),
	m_NextPID (USBPIDData0)
#endif
{
	assert (m_pDevice != 0);

	assert (pDesc != 0);
	assert (pDesc->bLength >= sizeof *pDesc);	// may have class-specific trailer
	assert (pDesc->bDescriptorType == DESCRIPTOR_ENDPOINT);

	switch (pDesc->bmAttributes & 0x03)
	{
	case 1:
		m_Type = EndpointTypeIsochronous;
		break;

	case 2:
		m_Type = EndpointTypeBulk;
		break;

	case 3:
		m_Type = EndpointTypeInterrupt;
		break;

	default:
		assert (0);	// endpoint configuration should be checked by function driver
		return;
	}
	
	m_ucNumber       = pDesc->bEndpointAddress & 0x0F;
	m_bDirectionIn   = pDesc->bEndpointAddress & 0x80 ? TRUE : FALSE;
	m_nMaxPacketSize = pDesc->wMaxPacketSize & 0x7FF;

#if RASPPI <= 3
	if (   m_Type == EndpointTypeInterrupt
	    || m_Type == EndpointTypeIsochronous)
	{
		u8 ucInterval = pDesc->bInterval;
		if (ucInterval < 1)
		{
			ucInterval = 1;
		}

		// see USB 2.0 spec chapter 9.6.6
		if (m_pDevice->GetSpeed () < USBSpeedHigh)
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

#ifndef USE_USB_SOF_INTR
		// interval 20ms is minimum to reduce interrupt rate
		if (m_nInterval < 20)
		{
			m_nInterval = 20;
		}
#endif
	}
#endif

	// workaround for low-speed devices with bulk endpoints,
	// which is normally forbidden by the USB spec.
	if (   m_pDevice->GetSpeed () == USBSpeedLow
	    && m_Type == EndpointTypeBulk)
	{
		CLogger::Get ()->Write ("uep", LogWarning, "Device is not fully USB compliant");

		m_Type = EndpointTypeInterrupt;

		if (m_nMaxPacketSize > 8)
		{
			m_nMaxPacketSize = 8;
		}

#if RASPPI <= 3
#ifdef USE_USB_SOF_INTR
		m_nInterval = 1;
#else
		m_nInterval = 20;
#endif
#endif
	}

#if RASPPI >= 4
	m_pXHCIEndpoint = new CXHCIEndpoint ((CXHCIUSBDevice *) m_pDevice, pDesc,
					     (CXHCIDevice *) m_pDevice->GetHost ());
#endif
}

CUSBEndpoint::~CUSBEndpoint (void)
{
#if RASPPI >= 4
	delete m_pXHCIEndpoint;
	m_pXHCIEndpoint = 0;
#endif

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

boolean CUSBEndpoint::SetMaxPacketSize (u32 nMaxPacketSize)
{
	m_nMaxPacketSize = nMaxPacketSize;

#if RASPPI >= 4
	assert (m_pXHCIEndpoint != 0);
	return m_pXHCIEndpoint->SetMaxPacketSize (nMaxPacketSize);
#else
	return TRUE;
#endif
}

u32 CUSBEndpoint::GetMaxPacketSize (void) const
{
	return m_nMaxPacketSize;
}

#if RASPPI <= 3

unsigned CUSBEndpoint::GetInterval (void) const
{
	assert (   m_Type == EndpointTypeInterrupt
		|| m_Type == EndpointTypeIsochronous);

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
	// TODO: for the supported Isochronous endpoints PID is always DATA0 (as initially set)
	if (m_Type == EndpointTypeIsochronous)
	{
		return;
	}

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

#endif

void CUSBEndpoint::ResetPID (void)
{
#if RASPPI <= 3
	assert (   m_Type == EndpointTypeControl
		|| m_Type == EndpointTypeBulk);

	m_NextPID = m_Type == EndpointTypeControl ? USBPIDSetup : USBPIDData0;
#endif
}

#if RASPPI >= 4

CXHCIEndpoint *CUSBEndpoint::GetXHCIEndpoint (void)
{
	assert (m_pXHCIEndpoint != 0);
	return m_pXHCIEndpoint;
}

#endif
