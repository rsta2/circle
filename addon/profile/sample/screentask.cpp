//
// screentask.h
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
#include "screentask.h"
#include <circle/sched/scheduler.h>
#include <circle/string.h>

CScreenTask::CScreenTask (unsigned nTaskID, CScreenDevice *pScreen)
:	m_nTaskID (nTaskID),
	m_pScreen (pScreen)
{
}

CScreenTask::~CScreenTask (void)
{
}

void CScreenTask::Run (void)
{
	while (1)
	{
		CString Message;
		Message.Format ("#%u   ", m_nTaskID);

		for (unsigned i = 0; i < m_nTaskID; i++)
		{
			Message.Append ("    ");
		}
		Message.Append ("****\n");

		m_pScreen->Write (Message, Message.GetLength ());

		CScheduler::Get ()->MsSleep (m_nTaskID * 1000);
	}
}
