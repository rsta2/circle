//
// touchscreen.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2021  R. Stange <rsta2@o2online.de>
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
#include <circle/input/touchscreen.h>
#include <circle/devicenameservice.h>
#include <assert.h>

CNumberPool CTouchScreenDevice::s_DeviceNumberPool (1);

static const char DevicePrefix[] = "touch";

CTouchScreenDevice::CTouchScreenDevice (TTouchScreenUpdateHandler *pUpdateHandler, void *pParam)
:	m_pUpdateHandler (pUpdateHandler),
	m_pUpdateParam (pParam),
	m_pEventHandler (0),
	m_nDeviceNumber (s_DeviceNumberPool.AllocateNumber (TRUE, DevicePrefix))
{
	CDeviceNameService::Get ()->AddDevice (DevicePrefix, m_nDeviceNumber, this, FALSE);
}

CTouchScreenDevice::~CTouchScreenDevice (void)
{
	m_pUpdateHandler = 0;
	m_pEventHandler = 0;

	CDeviceNameService::Get ()->RemoveDevice (DevicePrefix, m_nDeviceNumber, FALSE);

	s_DeviceNumberPool.FreeNumber (m_nDeviceNumber);
}

void CTouchScreenDevice::Update (void)
{
	if (m_pUpdateHandler != 0)
	{
		(*m_pUpdateHandler) (m_pUpdateParam);
	}
}

void CTouchScreenDevice::RegisterEventHandler (TTouchScreenEventHandler *pEventHandler)
{
	assert (m_pEventHandler == 0);
	m_pEventHandler = pEventHandler;
	assert (m_pEventHandler != 0);
}

void CTouchScreenDevice::ReportHandler (TTouchScreenEvent Event,
					unsigned nID, unsigned nPosX, unsigned nPosY)
{
	if (m_pEventHandler != 0)
	{
		(*m_pEventHandler) (Event, nID, nPosX, nPosY);
	}
}
