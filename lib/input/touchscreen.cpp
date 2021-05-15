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
	m_nScaleX (1000),
	m_nScaleY (1000),
	m_nOffsetX (0),
	m_nOffsetY (0),
	m_nWidth (10000),
	m_nHeight (10000),
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

boolean CTouchScreenDevice::SetCalibration (const unsigned Coords[4],
					    unsigned nWidth, unsigned nHeight)
{
	if (   Coords[0] >= Coords[1]
	    || Coords[2] >= Coords[3]
	    || nWidth == 0
	    || nHeight == 0)
	{
		return FALSE;
	}

	unsigned nTouchWidth = Coords[1] - Coords[0] + 1;
	unsigned nTouchHeight = Coords[3] - Coords[2] + 1;
	if (   nTouchWidth == 0
	    || nTouchHeight == 0)
	{
		return FALSE;
	}

	m_nScaleX = 1000 * nWidth / nTouchWidth;
	m_nOffsetX = Coords[0] * m_nScaleX/1000;

	m_nScaleY = 1000 * nHeight / nTouchHeight;
	m_nOffsetY = Coords[2] * m_nScaleY/1000;

	m_nWidth = nWidth;
	m_nHeight = nHeight;

	return TRUE;
}

void CTouchScreenDevice::ReportHandler (TTouchScreenEvent Event,
					unsigned nID, unsigned nPosX, unsigned nPosY)
{
	if (m_pEventHandler != 0)
	{
		if (Event != TouchScreenEventFingerUp)
		{
			nPosX = nPosX * m_nScaleX/1000 - m_nOffsetX;
			nPosY = nPosY * m_nScaleY/1000 - m_nOffsetY;

			if (   nPosX < m_nWidth
			    && nPosY < m_nHeight)
			{
				(*m_pEventHandler) (Event, nID, nPosX, nPosY);
			}
		}
		else
		{
			(*m_pEventHandler) (Event, nID, 0, 0);
		}
	}
}
