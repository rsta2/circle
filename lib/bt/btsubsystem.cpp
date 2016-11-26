//
// btsubsystem.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2016  R. Stange <rsta2@o2online.de>
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
#include <circle/bt/btsubsystem.h>
#include <circle/bt/bttask.h>
#include <circle/sched/scheduler.h>
#include <circle/devicenameservice.h>
#include <circle/machineinfo.h>
#include <assert.h>

CBTSubSystem::CBTSubSystem (CInterruptSystem *pInterruptSystem, u32 nClassOfDevice, const char *pLocalName)
:	m_pInterruptSystem (pInterruptSystem),
	m_pUARTTransport (0),
	m_HCILayer (nClassOfDevice, pLocalName),
	m_LogicalLayer (&m_HCILayer)
{
}

CBTSubSystem::~CBTSubSystem (void)
{
	delete m_pUARTTransport;
	m_pUARTTransport = 0;
}

boolean CBTSubSystem::Initialize (void)
{
	// if USB transport not available, UART still free and this is a RPi 3: use UART transport
	if (   CDeviceNameService::Get ()->GetDevice ("ubt1", FALSE) == 0
	    && CDeviceNameService::Get ()->GetDevice ("ttyS1", FALSE) == 0
	    && CMachineInfo::Get ()->GetModelMajor () == 3)
	{
		assert (m_pUARTTransport == 0);
		assert (m_pInterruptSystem != 0);
		m_pUARTTransport = new CBTUARTTransport (m_pInterruptSystem);
		assert (m_pUARTTransport != 0);

		if (!m_pUARTTransport->Initialize ())
		{
			return FALSE;
		}
	}

	if (!m_HCILayer.Initialize ())
	{
		return FALSE;
	}

	if (!m_LogicalLayer.Initialize ())
	{
		return FALSE;
	}

	new CBTTask (this);

	while (!m_HCILayer.GetDeviceManager ()->DeviceIsRunning ())
	{
		CScheduler::Get ()->Yield ();
	}

	return TRUE;
}

void CBTSubSystem::Process (void)
{
	m_HCILayer.Process ();

	m_LogicalLayer.Process ();
}

CBTInquiryResults *CBTSubSystem::Inquiry (unsigned nSeconds)
{
	return m_LogicalLayer.Inquiry (nSeconds);
}
