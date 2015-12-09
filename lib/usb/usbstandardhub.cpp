//
// usbstandardhub.cpp
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
#include <circle/usb/usbstandardhub.h>
#include <circle/usb/usbdevicefactory.h>
#include <circle/devicenameservice.h>
#include <circle/logger.h>
#include <circle/timer.h>
#include <circle/koptions.h>
#include <circle/debug.h>
#include <circle/macros.h>
#include <assert.h>

unsigned CUSBStandardHub::s_nDeviceNumber = 1;

static const char FromHub[] = "usbhub";

CUSBStandardHub::CUSBStandardHub (CUSBFunction *pFunction)
:	CUSBFunction (pFunction),
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
	if (GetNumEndpoints () != 1)
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

	if (!CUSBFunction::Configure ())
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

	CString DeviceName;
	DeviceName.Format ("uhub%u", s_nDeviceNumber++);
	CDeviceNameService::Get ()->AddDevice (DeviceName, this, FALSE);

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

	// m_pHubDesc->bPwrOn2PwrGood delay seems to be not enough for some devices,
	// so we use the maximum or a configured value here
	unsigned nMsDelay = 510;
	CKernelOptions *pOptions = CKernelOptions::Get ();
	if (pOptions != 0)
	{
		unsigned nUSBPowerDelay = pOptions->GetUSBPowerDelay ();
		if (nUSBPowerDelay != 0)
		{
			nMsDelay = nUSBPowerDelay;
		}
	}
	CTimer::Get ()->MsDelay (nMsDelay);

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
			Speed = USBSpeedLow;
		}
		else if (m_pStatus[nPort]->wPortStatus & PORT_HIGH_SPEED__MASK)
		{
			Speed = USBSpeedHigh;
		}
		else
		{
			Speed = USBSpeedFull;
		}

		CUSBDevice *pHubDevice = GetDevice ();
		assert (pHubDevice != 0);

		boolean bSplit     = pHubDevice->IsSplit ();
		u8 ucHubAddress    = pHubDevice->GetHubAddress ();
		u8 ucHubPortNumber = pHubDevice->GetHubPortNumber ();

		// Is this the first high-speed hub with a non-high-speed device following in chain?
		if (   !bSplit
		    && pHubDevice->GetSpeed () == USBSpeedHigh
		    && Speed < USBSpeedHigh)
		{
			// Then enable split transfers with this hub port as translator.
			bSplit          = TRUE;
			ucHubAddress    = pHubDevice->GetAddress ();
			ucHubPortNumber = nPort+1;
		}

		assert (m_pDevice[nPort] == 0);
		m_pDevice[nPort] = new CUSBDevice (pHost, Speed, bSplit, ucHubAddress, ucHubPortNumber);
		assert (m_pDevice[nPort] != 0);

		if (!m_pDevice[nPort]->Initialize ())
		{
			delete m_pDevice[nPort];
			m_pDevice[nPort] = 0;

			continue;
		}
	}

	// now configure devices
	for (unsigned nPort = 0; nPort < m_nPorts; nPort++)
	{
		if (m_pDevice[nPort] == 0)
		{
			continue;
		}

		if (!m_pDevice[nPort]->Configure ())
		{
			CLogger::Get ()->Write (FromHub, LogWarning,
						"Port %u: Cannot configure device", nPort+1);

			delete m_pDevice[nPort];
			m_pDevice[nPort] = 0;

			continue;
		}
		
		CLogger::Get ()->Write (FromHub, LogDebug, "Port %u: Device configured", nPort+1);
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
