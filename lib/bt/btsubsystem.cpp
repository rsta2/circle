//
// btsubsystem.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015  R. Stange <rsta2@o2online.de>
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

CBTSubSystem::CBTSubSystem (u32 nClassOfDevice, const char *pLocalName)
:	m_HCILayer (nClassOfDevice, pLocalName),
	m_LogicalLayer (&m_HCILayer)
{
}

CBTSubSystem::~CBTSubSystem (void)
{
}

boolean CBTSubSystem::Initialize (void)
{
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
