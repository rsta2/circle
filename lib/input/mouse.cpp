//
// mouse.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2018  R. Stange <rsta2@o2online.de>
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
#include <circle/input/mouse.h>
#include <circle/devicenameservice.h>
#include <assert.h>

unsigned CMouseDevice::s_nDeviceNumber = 1;

static const char DevicePrefix[] = "mouse";

CMouseDevice::CMouseDevice (void)
:	m_pStatusHandler (0),
	m_nDeviceNumber (s_nDeviceNumber++)
{
	CDeviceNameService::Get ()->AddDevice (DevicePrefix, m_nDeviceNumber, this, FALSE);
}

CMouseDevice::~CMouseDevice (void)
{
	m_pStatusHandler = 0;

	CDeviceNameService::Get ()->RemoveDevice (DevicePrefix, m_nDeviceNumber, FALSE);
}

boolean CMouseDevice::Setup (unsigned nScreenWidth, unsigned nScreenHeight)
{
	return m_Behaviour.Setup (nScreenWidth, nScreenHeight);
}

void CMouseDevice::RegisterEventHandler (TMouseEventHandler *pEventHandler)
{
	m_Behaviour.RegisterEventHandler (pEventHandler);
}

boolean CMouseDevice::SetCursor (unsigned nPosX, unsigned nPosY)
{
	return m_Behaviour.SetCursor (nPosX, nPosY);
}

boolean CMouseDevice::ShowCursor (boolean bShow)
{
	return m_Behaviour.ShowCursor (bShow);
}

void CMouseDevice::UpdateCursor (void)
{
	if (m_pStatusHandler == 0)
	{
		m_Behaviour.UpdateCursor ();
	}
}

void CMouseDevice::RegisterStatusHandler (TMouseStatusHandler *pStatusHandler)
{
	assert (m_pStatusHandler == 0);
	m_pStatusHandler = pStatusHandler;
	assert (m_pStatusHandler != 0);
}

void CMouseDevice::ReportHandler (unsigned nButtons, int nDisplacementX, int nDisplacementY)
{
	m_Behaviour.MouseStatusChanged (nButtons, nDisplacementX, nDisplacementY);

	if (m_pStatusHandler != 0)
	{
		(*m_pStatusHandler) (nButtons, nDisplacementX, nDisplacementY);
	}
}
