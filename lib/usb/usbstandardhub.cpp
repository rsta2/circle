//
// usbstandardhub.cpp
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
#include <circle/usb/usbstandardhub.h>
#include <circle/usb/usbdevicefactory.h>
#include <circle/logger.h>
#include <circle/timer.h>
#include <circle/debug.h>
#include <circle/macros.h>
#include <assert.h>

static const char FromHub[] = "usbhub";

CUSBStandardHub::CUSBStandardHub (CUSBHostController *pHost, TUSBSpeed Speed)
:	CUSBDevice (pHost, Speed, 0, 1),
	m_pHubDesc (0),
	m_nPorts (0)
{
	for (unsigned nPort = 0; nPort < USB_HUB_MAX_PORTS; nPort++)
	{
		m_pDevice[nPort] = 0;
		m_pStatus[nPort] = 0;
	}
}

CUSBStandardHub::CUSBStandardHub (CUSBDevice *pDevice)
:	CUSBDevice (pDevice),
	m_pHubDesc (0),
	m_nPorts (0)
{
	for (unsigned nPort = 0; nPort < USB_HUB_MAX_PORTS; nPort++)
	{
		m_pDevice[nPort] = 0;
		m_pStatus[nPort] = 0;
	}
}

CUSBStandardHub::~CUSBStandardHub (void)
{
	for (unsigned nPort = 0; nPort < m_nPorts; nPort++)
	{
		delete m_pStatus[nPort];
		m_pStatus[nPort] = 0;
		
		delete m_pDevice[nPort];
		m_pDevice[nPort] = 0;
	}

	m_nPorts = 0;

	delete m_pHubDesc;
	m_pHubDesc = 0;
}

boolean CUSBStandardHub::Configure (void)
{
	const TUSBDeviceDescriptor *pDeviceDesc = GetDeviceDescriptor ();
	assert (pDeviceDesc != 0);

	if (   pDeviceDesc->bDeviceClass       != USB_DEVICE_CLASS_HUB
	    || pDeviceDesc->bDeviceSubClass    != 0
	    || pDeviceDesc->bDeviceProtocol    != 2		// hub with multiple TTs
	    || pDeviceDesc->bNumConfigurations != 1)
	{
		CLogger::Get ()->Write (FromHub, LogError, "Unsupported hub (proto %u)",
					(unsigned) pDeviceDesc->bDeviceProtocol);

		return FALSE;
	}

	const TUSBConfigurationDescriptor *pConfigDesc =
		(TUSBConfigurationDescriptor *) GetDescriptor (DESCRIPTOR_CONFIGURATION);
	if (   pConfigDesc == 0
	    || pConfigDesc->bNumInterfaces != 1)
	{
		ConfigurationError (FromHub);

		return FALSE;
	}

	const TUSBInterfaceDescriptor *pInterfaceDesc;
	while ((pInterfaceDesc = (TUSBInterfaceDescriptor *) GetDescriptor (DESCRIPTOR_INTERFACE)) != 0)
	{
		if (   pInterfaceDesc->bInterfaceClass    != USB_DEVICE_CLASS_HUB
		    || pInterfaceDesc->bInterfaceSubClass != 0
		    || pInterfaceDesc->bInterfaceProtocol != 2)
		{
			continue;
		}

		if (pInterfaceDesc->bNumEndpoints != 1)
		{
			ConfigurationError (FromHub);

			return FALSE;
		}

		const TUSBEndpointDescriptor *pEndpointDesc =
			(TUSBEndpointDescriptor *) GetDescriptor (DESCRIPTOR_ENDPOINT);
		if (   pEndpointDesc == 0
		    || (pEndpointDesc->bEndpointAddress & 0x80) != 0x80		// input EP
		    || (pEndpointDesc->bmAttributes     & 0x3F) != 0x03)	// interrupt EP
		{
			ConfigurationError (FromHub);

			return FALSE;
		}

		break;
	}

	if (pInterfaceDesc == 0)
	{
		ConfigurationError (FromHub);

		return FALSE;
	}

	if (!CUSBDevice::Configure ())
	{
		CLogger::Get ()->Write (FromHub, LogError, "Cannot set configuration");

		return FALSE;
	}

	if (GetHost ()->ControlMessage (GetEndpoint0 (),
					REQUEST_OUT | REQUEST_TO_INTERFACE, SET_INTERFACE,
					pInterfaceDesc->bAlternateSetting,
					pInterfaceDesc->bInterfaceNumber, 0, 0) < 0)
	{
		CLogger::Get ()->Write (FromHub, LogError, "Cannot set interface");

		return FALSE;
	}

	assert (m_pHubDesc == 0);
	m_pHubDesc = new TUSBHubDescriptor;
	assert (m_pHubDesc != 0);

	if (GetHost ()->GetDescriptor (GetEndpoint0 (),
					DESCRIPTOR_HUB, DESCRIPTOR_INDEX_DEFAULT,
					m_pHubDesc, sizeof *m_pHubDesc,
					REQUEST_IN | REQUEST_CLASS)
	   != (int) sizeof *m_pHubDesc)
	{
		CLogger::Get ()->Write (FromHub, LogError, "Cannot get hub descriptor");
		
		delete m_pHubDesc;
		m_pHubDesc = 0;
		
		return FALSE;
	}

#ifndef NDEBUG
	//debug_hexdump (m_pHubDesc, sizeof *m_pHubDesc, FromHub);
#endif

	m_nPorts = m_pHubDesc->bNbrPorts;
	if (m_nPorts > USB_HUB_MAX_PORTS)
	{
		CLogger::Get ()->Write (FromHub, LogError, "Too many ports (%u)", m_nPorts);
		
		delete m_pHubDesc;
		m_pHubDesc = 0;
		
		return FALSE;
	}

	if (!EnumeratePorts ())
	{
		CLogger::Get ()->Write (FromHub, LogError, "Port enumeration failed");

		return FALSE;
	}

	return TRUE;
}

boolean CUSBStandardHub::EnumeratePorts (void)
{
	CUSBHostController *pHost = GetHost ();
	assert (pHost != 0);
	
	CUSBEndpoint *pEndpoint0 = GetEndpoint0 ();
	assert (pEndpoint0 != 0);

	assert (m_nPorts > 0);

	// first power on all ports
	for (unsigned nPort = 0; nPort < m_nPorts; nPort++)
	{
		if (pHost->ControlMessage (pEndpoint0,
			REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_OTHER,
			SET_FEATURE, PORT_POWER, nPort+1, 0, 0) < 0)
		{
			CLogger::Get ()->Write (FromHub, LogError, "Cannot power port %u", nPort+1);

			return FALSE;
		}
	}

	// m_pHubDesc->bPwrOn2PwrGood delay seems to be not enough
	// for some low speed devices, so we use the maximum here
	CTimer::Get ()->MsDelay (510);

	// now detect devices, reset and initialize them
	for (unsigned nPort = 0; nPort < m_nPorts; nPort++)
	{
		assert (m_pStatus[nPort] == 0);
		m_pStatus[nPort] = new TUSBPortStatus;
		assert (m_pStatus[nPort] != 0);

		if (pHost->ControlMessage (pEndpoint0,
			REQUEST_IN | REQUEST_CLASS | REQUEST_TO_OTHER,
			GET_STATUS, 0, nPort+1, m_pStatus[nPort], 4) != 4)
		{
			CLogger::Get ()->Write (FromHub, LogError,
						"Cannot get status of port %u", nPort+1);
		
			continue;
		}

		assert (m_pStatus[nPort]->wPortStatus & PORT_POWER__MASK);
		if (!(m_pStatus[nPort]->wPortStatus & PORT_CONNECTION__MASK))
		{
			continue;
		}

		if (pHost->ControlMessage (pEndpoint0,
			REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_OTHER,
			SET_FEATURE, PORT_RESET, nPort+1, 0, 0) < 0)
		{
			CLogger::Get ()->Write (FromHub, LogError, "Cannot reset port %u", nPort+1);

			continue;
		}

		CTimer::Get ()->MsDelay (100);
		
		if (pHost->ControlMessage (pEndpoint0,
			REQUEST_IN | REQUEST_CLASS | REQUEST_TO_OTHER,
			GET_STATUS, 0, nPort+1, m_pStatus[nPort], 4) != 4)
		{
			return FALSE;
		}

		//CLogger::Get ()->Write (FromHub, LogDebug, "Port %u status is 0x%04X", nPort+1, (unsigned) m_pStatus[nPort]->wPortStatus);
		
		if (!(m_pStatus[nPort]->wPortStatus & PORT_ENABLE__MASK))
		{
			CLogger::Get ()->Write (FromHub, LogError,
						"Port %u is not enabled", nPort+1);

			continue;
		}

		// check for over-current
		if (m_pStatus[nPort]->wPortStatus & PORT_OVER_CURRENT__MASK)
		{
			pHost->ControlMessage (pEndpoint0,
				REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_OTHER,
				CLEAR_FEATURE, PORT_POWER, nPort+1, 0, 0);

			CLogger::Get ()->Write (FromHub, LogError,
						"Over-current condition on port %u", nPort+1);

			return FALSE;
		}

		TUSBSpeed Speed = USBSpeedUnknown;
		if (m_pStatus[nPort]->wPortStatus & PORT_LOW_SPEED__MASK)
		{
			//Speed = USBSpeedLow;
			CLogger::Get ()->Write (FromHub, LogWarning, "Port %u: Low-speed devices are not supported at the moment", nPort+1);
			continue;
		}
		else if (m_pStatus[nPort]->wPortStatus & PORT_HIGH_SPEED__MASK)
		{
			Speed = USBSpeedHigh;
		}
		else
		{
			//Speed = USBSpeedFull;
			CLogger::Get ()->Write (FromHub, LogWarning, "Port %u: Full-speed devices are not supported at the moment", nPort+1);
			continue;
		}

		// first create default device
		assert (m_pDevice[nPort] == 0);
		m_pDevice[nPort] = new CUSBDevice (pHost, Speed, GetAddress (), nPort+1);
		assert (m_pDevice[nPort] != 0);

		if (!m_pDevice[nPort]->Initialize ())
		{
			delete m_pDevice[nPort];
			m_pDevice[nPort] = 0;

			continue;
		}

		CString *pNames = GetDeviceNames (m_pDevice[nPort]);
		assert (pNames != 0);

		CLogger::Get ()->Write (FromHub, LogNotice,
					"Port %u: Device %s found", nPort+1, (const char *) *pNames);

		delete pNames;
	}

	// now configure devices
	for (unsigned nPort = 0; nPort < m_nPorts; nPort++)
	{
		if (m_pDevice[nPort] == 0)
		{
			continue;
		}

		// now create specific device from default device
		CUSBDevice *pChild = CUSBDeviceFactory::GetDevice (m_pDevice[nPort]);
		if (pChild != 0)
		{
			delete m_pDevice[nPort];		// delete default device
			m_pDevice[nPort] = pChild;		// assign specific device

			if (!m_pDevice[nPort]->Configure ())
			{
				CLogger::Get ()->Write (FromHub, LogError,
							"Port %u: Cannot configure device", nPort+1);

				continue;
			}
			
			CLogger::Get ()->Write (FromHub, LogDebug, "Port %u: Device configured", nPort+1);
		}
		else
		{
			CLogger::Get ()->Write (FromHub, LogNotice,
						"Port %u: Device is not supported", nPort+1);
			
			delete m_pDevice[nPort];
			m_pDevice[nPort] = 0;
		}
	}

	// again check for over-current
	TUSBHubStatus *pHubStatus = new TUSBHubStatus;
	assert (pHubStatus != 0);

	if (pHost->ControlMessage (pEndpoint0,
		REQUEST_IN | REQUEST_CLASS,
		GET_STATUS, 0, 0, pHubStatus, sizeof *pHubStatus) != (int) sizeof *pHubStatus)
	{
		CLogger::Get ()->Write (FromHub, LogError, "Cannot get hub status");

		delete pHubStatus;

		return FALSE;
	}

	if (pHubStatus->wHubStatus & HUB_OVER_CURRENT__MASK)
	{
		for (unsigned nPort = 0; nPort < m_nPorts; nPort++)
		{
			pHost->ControlMessage (pEndpoint0,
				REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_OTHER,
				CLEAR_FEATURE, PORT_POWER, nPort+1, 0, 0);
		}

		CLogger::Get ()->Write (FromHub, LogError, "Hub over-current condition");

		delete pHubStatus;

		return FALSE;
	}

	delete pHubStatus;
	pHubStatus = 0;

	boolean bResult = TRUE;

	for (unsigned nPort = 0; nPort < m_nPorts; nPort++)
	{
		if (pHost->ControlMessage (pEndpoint0,
			REQUEST_IN | REQUEST_CLASS | REQUEST_TO_OTHER,
			GET_STATUS, 0, nPort+1, m_pStatus[nPort], 4) != 4)
		{
			continue;
		}

		if (m_pStatus[nPort]->wPortStatus & PORT_OVER_CURRENT__MASK)
		{
			pHost->ControlMessage (pEndpoint0,
				REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_OTHER,
				CLEAR_FEATURE, PORT_POWER, nPort+1, 0, 0);

			CLogger::Get ()->Write (FromHub, LogError,
						"Over-current condition on port %u", nPort+1);

			bResult = FALSE;
		}
	}

	return bResult;
}

CString *CUSBStandardHub::GetDeviceNames (CUSBDevice *pDevice)
{
	assert (pDevice != 0);
	
	CString *pResult = new CString;

	for (unsigned nSelector = DeviceNameVendor; nSelector < DeviceNameUnknown; nSelector++)
	{
		CString *pName = pDevice->GetName ((TDeviceNameSelector) nSelector);
		assert (pName != 0);

		if (pName->Compare ("unknown") != 0)
		{
			if (pResult->GetLength () > 0)
			{
				pResult->Append (", ");
			}

			pResult->Append (*pName);
		}
		
		delete pName;
	}

	if (pResult->GetLength () == 0)
	{
		*pResult = "unknown";
	}

	return pResult;
}
