//
// nettask.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2021  R. Stange <rsta2@o2online.de>
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
#include <circle/net/nettask.h>
#include <circle/sched/scheduler.h>
#include <assert.h>

CNetTask::CNetTask (CNetSubSystem *pNetSubSystem)
:	m_pNetSubSystem (pNetSubSystem)
{
	SetName ("net");
}

CNetTask::~CNetTask (void)
{
	m_pNetSubSystem = 0;
}

void CNetTask::Run (void)
{
	while (1)
	{
		assert (m_pNetSubSystem != 0);
		m_pNetSubSystem->Process ();

		CScheduler::Get ()->Yield ();
	}
}
