//
// xhcirootport.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2019-2020  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/xhcirootport.h>
#include <circle/usb/xhcidevice.h>
#include <circle/logger.h>
#include <assert.h>

static const char From[] = "xhciroot";

CXHCIRootPort::CXHCIRootPort (u8 uchPortID, CXHCIDevice *pXHCIDevice)
:	m_nPortIndex (uchPortID-1),
	m_pXHCIDevice (pXHCIDevice),
	m_pMMIO (pXHCIDevice->GetMMIOSpace ()),
	m_pUSBDevice (0)
{
	assert (XHCI_IS_PORTID (uchPortID));
}

CXHCIRootPort::~CXHCIRootPort (void)
{
	delete m_pUSBDevice;
	m_pUSBDevice = 0;

	m_pMMIO = 0;
	m_pXHCIDevice = 0;
}

boolean CXHCIRootPort::Initialize (void)
{
	if (!IsConnected ())
	{
		return FALSE;
	}

	assert (m_nPortIndex < XHCI_CONFIG_MAX_PORTS);
	if (XHCI_IS_USB2_PORT (m_nPortIndex+1))
	{
		if (!Reset (100000))
		{
			CLogger::Get ()->Write (From, LogWarning,
						"Port %u: Reset failed", m_nPortIndex+1);

			return FALSE;
		}
	}
	else
	{
		if (!WaitForU0State (100000))
		{
			CLogger::Get ()->Write (From, LogWarning,
						"Port %u: Wrong state", m_nPortIndex+1);

			return FALSE;
		}
	}

	assert (m_pUSBDevice == 0);
	m_pUSBDevice = new CXHCIUSBDevice (m_pXHCIDevice, GetPortSpeed (), this);
	assert (m_pUSBDevice != 0);

	if (!m_pUSBDevice->Initialize ())
	{
		delete m_pUSBDevice;
		m_pUSBDevice = 0;

		return FALSE;
	}

	if (PowerOffOnOverCurrent ())
	{
		CLogger::Get ()->Write (From, LogError,
					"Port %u: Over-current detected", m_nPortIndex+1);

		delete m_pUSBDevice;
		m_pUSBDevice = 0;

		return FALSE;
	}

#ifdef XHCI_DEBUG
	CLogger::Get ()->Write (From, LogDebug, "Port %u: Device initialized", m_nPortIndex+1);
#endif

	return TRUE;
}

boolean CXHCIRootPort::Configure (void)
{
	if (m_pUSBDevice == 0)
	{
		return FALSE;
	}

	if (!m_pUSBDevice->Configure ())
	{
		CLogger::Get ()->Write (From, LogWarning, "Cannot configure device");

		delete m_pUSBDevice;
		m_pUSBDevice = 0;

		return FALSE;
	}

	if (PowerOffOnOverCurrent ())
	{
		CLogger::Get ()->Write (From, LogError,
					"Port %u: Over-current detected", m_nPortIndex+1);

		delete m_pUSBDevice;
		m_pUSBDevice = 0;

		return FALSE;
	}

	CLogger::Get ()->Write (From, LogDebug,
				"Port %u: Device configured", m_nPortIndex+1);

	return TRUE;
}

u8 CXHCIRootPort::GetPortID (void) const
{
	assert (m_nPortIndex < XHCI_CONFIG_MAX_PORTS);

	return (u8) (m_nPortIndex+1);
}

TUSBSpeed CXHCIRootPort::GetPortSpeed (void)
{
	assert (m_pMMIO != 0);
	assert (m_nPortIndex < XHCI_CONFIG_MAX_PORTS);
	u32 nPortSC = m_pMMIO->pt_read32 (m_nPortIndex, XHCI_REG_OP_PORTS_PORTSC);

	u8 uchSpeedID =     (nPortSC & XHCI_REG_OP_PORTS_PORTSC_PORT_SPEED__MASK)
			 >> XHCI_REG_OP_PORTS_PORTSC_PORT_SPEED__SHIFT;
	if (   uchSpeedID == 0
	    || !XHCI_IS_PSI (uchSpeedID))
	{
		return USBSpeedUnknown;
	}

	return XHCI_PSI_TO_USB_SPEED (uchSpeedID);
}

boolean CXHCIRootPort::ReScanDevices (void)
{
	if (m_pUSBDevice != 0)
	{
		return m_pUSBDevice->ReScanDevices ();
	}

	if (!Initialize ())
	{
		return FALSE;
	}

	if (!Configure ())
	{
		return FALSE;
	}

	return TRUE;
}

boolean CXHCIRootPort::RemoveDevice (void)
{
	delete m_pUSBDevice;
	m_pUSBDevice = 0;

	return TRUE;
}

void CXHCIRootPort::HandlePortStatusChange (void)
{
	if (IsConnected ())
	{
		if (m_pUSBDevice == 0)
		{
			ReScanDevices ();
		}
	}
	else
	{
		if (m_pUSBDevice != 0)
		{
			RemoveDevice ();
		}
	}
}

void CXHCIRootPort::StatusChanged (void)
{
	assert (m_pMMIO != 0);
	assert (m_nPortIndex < XHCI_CONFIG_MAX_PORTS);

	m_SpinLock.Acquire ();

	u32 nPortSC = m_pMMIO->pt_read32 (m_nPortIndex, XHCI_REG_OP_PORTS_PORTSC);

#ifdef XHCI_DEBUG
	CLogger::Get ()->Write (From, LogDebug, "Port %u status is 0x%X", m_nPortIndex+1, nPortSC);
#endif

	m_pMMIO->pt_write32 (m_nPortIndex, XHCI_REG_OP_PORTS_PORTSC,	// clear port status
			     nPortSC & ~XHCI_REG_OP_PORTS_PORTSC_PED);	// do not disable port

	m_SpinLock.Release ();

	assert (m_pXHCIDevice != 0);
	if (   m_pXHCIDevice->IsPlugAndPlay ()
	    && (nPortSC & XHCI_REG_OP_PORTS_PORTSC_CSC))
	{
		m_pXHCIDevice->PortStatusChanged (this);
	}
}

#ifndef NDEBUG

void CXHCIRootPort::DumpStatus (void)
{
	CLogger::Get ()->Write (From, LogDebug, "Port %u: Status 0x%X, Info 0x%X", m_nPortIndex+1,
				m_pMMIO->pt_read32 (m_nPortIndex, XHCI_REG_OP_PORTS_PORTSC),
				m_pMMIO->pt_read32 (m_nPortIndex, XHCI_REG_OP_PORTS_PORTLI));

	if (m_pUSBDevice != 0)
	{
		m_pUSBDevice->DumpStatus ();
	}
}

#endif

boolean CXHCIRootPort::IsConnected (void)
{
	assert (m_pMMIO != 0);
	assert (m_nPortIndex < XHCI_CONFIG_MAX_PORTS);
	u32 nPortSC = m_pMMIO->pt_read32 (m_nPortIndex, XHCI_REG_OP_PORTS_PORTSC);

	return !!(nPortSC & XHCI_REG_OP_PORTS_PORTSC_CCS);
}

boolean CXHCIRootPort::Reset (unsigned nTimeoutUsecs)
{
	assert (m_pMMIO != 0);
	assert (m_nPortIndex < XHCI_CONFIG_MAX_PORTS);

	m_SpinLock.Acquire ();

	u32 nPortSC = m_pMMIO->pt_read32 (m_nPortIndex, XHCI_REG_OP_PORTS_PORTSC);
	nPortSC |= XHCI_REG_OP_PORTS_PORTSC_PR;
	m_pMMIO->pt_write32 (m_nPortIndex, XHCI_REG_OP_PORTS_PORTSC,
			     nPortSC & ~XHCI_REG_OP_PORTS_PORTSC_PED);

	m_SpinLock.Release ();

	return m_pMMIO->op_wait32 (  XHCI_REG_OP_PORTS_BASE
				   + m_nPortIndex*XHCI_REG_OP_PORTS_PORT__SIZE
				   + XHCI_REG_OP_PORTS_PORTSC,
				   XHCI_REG_OP_PORTS_PORTSC_PR, 0, nTimeoutUsecs);
}

boolean CXHCIRootPort::WaitForU0State (unsigned nTimeoutUsecs)
{
	assert (m_pMMIO != 0);
	assert (m_nPortIndex < XHCI_CONFIG_MAX_PORTS);

	return m_pMMIO->op_wait32 (  XHCI_REG_OP_PORTS_BASE
				   + m_nPortIndex*XHCI_REG_OP_PORTS_PORT__SIZE
				   + XHCI_REG_OP_PORTS_PORTSC,
				   XHCI_REG_OP_PORTS_PORTSC_PLS__MASK,
				      XHCI_REG_OP_PORTS_PORTSC_PLS_U0
				   << XHCI_REG_OP_PORTS_PORTSC_PLS__SHIFT, nTimeoutUsecs);
}

boolean CXHCIRootPort::PowerOffOnOverCurrent (void)
{
	assert (m_pMMIO != 0);
	assert (m_nPortIndex < XHCI_CONFIG_MAX_PORTS);

	m_SpinLock.Acquire ();

	u32 nPortSC = m_pMMIO->pt_read32 (m_nPortIndex, XHCI_REG_OP_PORTS_PORTSC);
	if (!(nPortSC & XHCI_REG_OP_PORTS_PORTSC_OCA))
	{
		m_SpinLock.Release ();

		return FALSE;
	}

	nPortSC &= ~XHCI_REG_OP_PORTS_PORTSC_PP;
	m_pMMIO->pt_write32 (m_nPortIndex, XHCI_REG_OP_PORTS_PORTSC, nPortSC);

	m_SpinLock.Release ();

	return TRUE;
}
