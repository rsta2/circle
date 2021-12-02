//
// phytask.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2019-2021  R. Stange <rsta2@o2online.de>
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
#include <circle/net/phytask.h>
#include <circle/sched/scheduler.h>
#include <assert.h>

CPHYTask::CPHYTask (CNetDevice *pDevice)
:	m_pDevice (pDevice)
{
	SetName ("netphy");
}

CPHYTask::~CPHYTask (void)
{
	m_pDevice = 0;
}

void CPHYTask::Run (void)
{
	while (1)
	{
		assert (m_pDevice != 0);
		if (!m_pDevice->UpdatePHY ())
		{
			return;
		}

		CScheduler::Get ()->MsSleep (2000);
	}
}
