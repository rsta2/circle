//
// mouse.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2023  R. Stange <rsta2@o2online.de>
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

CNumberPool CMouseDevice::s_DeviceNumberPool (1);

static const char FromMouse[] = "mouse";
static const char DevicePrefix[] = "mouse";

CMouseDevice::CMouseDevice (unsigned nButtons, boolean bHasWheel)
:	m_pStatusHandler (0),
	m_pStatusHandlerArg (0),
	m_nDeviceNumber (s_DeviceNumberPool.AllocateNumber (TRUE, FromMouse)),
	m_nButtons (nButtons),
	m_bHasWheel (bHasWheel)
{
	CDeviceNameService::Get ()->AddDevice (DevicePrefix, m_nDeviceNumber, this, FALSE);
}

CMouseDevice::~CMouseDevice (void)
{
	m_pStatusHandler = 0;
	m_pStatusHandlerArg = 0;

	CDeviceNameService::Get ()->RemoveDevice (DevicePrefix, m_nDeviceNumber, FALSE);

	s_DeviceNumberPool.FreeNumber (m_nDeviceNumber);
}

boolean CMouseDevice::Setup (unsigned nScreenWidth, unsigned nScreenHeight)
{
	return m_Behaviour.Setup (nScreenWidth, nScreenHeight);
}

void CMouseDevice::Release (void)
{
	m_Behaviour.Release ();
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

void CMouseDevice::RegisterStatusHandler (TMouseStatusHandlerEx *pStatusHandler, void* pArg)
{
	assert (m_pStatusHandler == 0);
	m_pStatusHandler = pStatusHandler;
	m_pStatusHandlerArg = pArg;
	assert (m_pStatusHandler != 0);
}

static void proxy_handler(unsigned nButtons, int nDisplacementX, int nDisplacementY, int nWheelMove, void* pArg)
{
	((TMouseStatusHandler*)pArg)(nButtons, nDisplacementX, nDisplacementY, nWheelMove);
}


void CMouseDevice::RegisterStatusHandler (TMouseStatusHandler *pStatusHandler)
{
	RegisterStatusHandler(proxy_handler, (void*)pStatusHandler);
}


void CMouseDevice::ReportHandler (unsigned nButtons, int nDisplacementX, int nDisplacementY, int nWheelMove)
{
	m_Behaviour.MouseStatusChanged (nButtons, nDisplacementX, nDisplacementY, nWheelMove);

	if (m_pStatusHandler != 0)
	{
		(*m_pStatusHandler) (nButtons, nDisplacementX, nDisplacementY, nWheelMove, m_pStatusHandlerArg);
	}
}

unsigned CMouseDevice::GetButtonCount (void) const
{
	return m_nButtons;
}

boolean CMouseDevice::HasWheel (void) const
{
	return m_bHasWheel;
}
