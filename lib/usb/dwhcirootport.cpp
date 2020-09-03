//
// dwhcirootport.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/dwhcirootport.h>
#include <circle/usb/dwhcidevice.h>
#include <circle/logger.h>
#include <assert.h>

static const char FromDWHCIRoot[] = "dwroot";

CDWHCIRootPort::CDWHCIRootPort (CDWHCIDevice *pHost)
:	m_pHost (pHost),
	m_pDevice (0)
{
	assert (m_pHost != 0);
}

CDWHCIRootPort::~CDWHCIRootPort (void)
{
	delete m_pDevice;
	m_pDevice = 0;

	m_pHost = 0;
}

boolean CDWHCIRootPort::Initialize (void)
{
	assert (m_pHost != 0);
	TUSBSpeed Speed = m_pHost->GetPortSpeed ();
	if (Speed == USBSpeedUnknown)
	{
		CLogger::Get ()->Write (FromDWHCIRoot, LogError, "Cannot detect port speed");

		return FALSE;
	}
	
	// first create default device
	assert (m_pDevice == 0);
	m_pDevice = new CUSBDevice (m_pHost, Speed, this);
	assert (m_pDevice != 0);

	if (!m_pDevice->Initialize ())
	{
		delete m_pDevice;
		m_pDevice = 0;

		return FALSE;
	}

	if (!m_pDevice->Configure ())
	{
		CLogger::Get ()->Write (FromDWHCIRoot, LogWarning, "Cannot configure device");

		delete m_pDevice;
		m_pDevice = 0;

		return FALSE;
	}

	CLogger::Get ()->Write (FromDWHCIRoot, LogDebug, "Device configured");

	// check for over-current
	if (m_pHost->OvercurrentDetected ())
	{
		CLogger::Get ()->Write (FromDWHCIRoot, LogError, "Over-current condition");

		m_pHost->DisableRootPort ();

		delete m_pDevice;
		m_pDevice = 0;

		return FALSE;
	}

	return TRUE;
}

boolean CDWHCIRootPort::ReScanDevices (void)
{
	if (m_pDevice == 0)
	{
		CLogger::Get ()->Write (FromDWHCIRoot, LogWarning,
					"Previous attempt to initialize device failed");

		return FALSE;
	}

	return m_pDevice->ReScanDevices ();
}

boolean CDWHCIRootPort::RemoveDevice (void)
{
	assert (m_pHost != 0);
	m_pHost->DisableRootPort (FALSE);

	delete m_pDevice;
	m_pDevice = 0;

	return TRUE;
}

void CDWHCIRootPort::HandlePortStatusChange (void)
{
	assert (m_pHost != 0);

	if (m_pHost->DeviceConnected ())
	{
		if (m_pDevice == 0)
		{
			m_pHost->ReScanDevices ();
		}
	}
	else
	{
		if (m_pDevice != 0)
		{
			RemoveDevice ();
		}
	}
}
