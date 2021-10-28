//
// uguicpp.cpp
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
#include <ugui/uguicpp.h>
#include <circle/devicenameservice.h>
#include <circle/koptions.h>
#include <circle/timer.h>
#include <assert.h>

CUGUI *CUGUI::s_pThis = 0;

CUGUI::CUGUI (CScreenDevice *pScreen)
:	m_pScreen (pScreen),
	m_pMouseDevice (0),
	m_pTouchScreen (0),
	m_nLastUpdate (0)
{
	assert (s_pThis == 0);
	s_pThis = this;
}

CUGUI::~CUGUI (void)
{
	m_pTouchScreen = 0;
	m_pMouseDevice = 0;
	m_pScreen =  0;

	s_pThis = 0;
}

boolean CUGUI::Initialize (void)
{
	assert (m_pScreen != 0);
	if (UG_Init (&m_GUI, SetPixel,
		     (UG_S16) m_pScreen->GetWidth (),
		     (UG_S16) m_pScreen->GetHeight ()) < 0)
	{
		return FALSE;
	}

	if (UG_SelectGUI (&m_GUI) < 0)
	{
		return FALSE;
	}

	m_pMouseDevice = (CMouseDevice *) CDeviceNameService::Get ()->GetDevice ("mouse1", FALSE);
	if (m_pMouseDevice != 0)
	{
		if (m_pMouseDevice->Setup (m_pScreen->GetWidth (), m_pScreen->GetHeight ()))
		{
			m_pMouseDevice->ShowCursor (TRUE);

			m_pMouseDevice->RegisterEventHandler (MouseEventStub);

			m_pMouseDevice->RegisterRemovedHandler (MouseRemovedHandler);
		}
	}

	m_pTouchScreen = (CTouchScreenDevice *) CDeviceNameService::Get ()->GetDevice ("touch1", FALSE);
	if (m_pTouchScreen != 0)
	{
		const unsigned *pCalibration = CKernelOptions::Get ()->GetTouchScreen ();
		if (pCalibration != 0)
		{
			m_pTouchScreen->SetCalibration (pCalibration, m_pScreen->GetWidth (),
							m_pScreen->GetHeight ());
		}

		m_pTouchScreen->RegisterEventHandler (TouchScreenEventStub);
	}

	UG_TouchUpdate (-1, -1, TOUCH_STATE_RELEASED);

	return TRUE;
}

void CUGUI::Update (boolean bPlugAndPlayUpdated)
{
	if (   bPlugAndPlayUpdated
	    && m_pMouseDevice == 0)
	{
		m_pMouseDevice =
			(CMouseDevice *) CDeviceNameService::Get ()->GetDevice ("mouse1", FALSE);
		if (m_pMouseDevice != 0)
		{
			assert (m_pScreen != 0);
			if (m_pMouseDevice->Setup (m_pScreen->GetWidth (), m_pScreen->GetHeight ()))
			{
				m_pMouseDevice->ShowCursor (TRUE);

				m_pMouseDevice->RegisterEventHandler (MouseEventStub);

				m_pMouseDevice->RegisterRemovedHandler (MouseRemovedHandler);
			}
		}
	}

	UG_Update ();

	if (m_pMouseDevice != 0)
	{
		m_pMouseDevice->UpdateCursor ();
	}

	if (m_pTouchScreen != 0)
	{
		unsigned nTicks = CTimer::Get ()->GetClockTicks ();
		if (nTicks - m_nLastUpdate >= CLOCKHZ/60)
		{
			m_pTouchScreen->Update ();

			m_nLastUpdate = nTicks;
		}
	}
}

void CUGUI::SetPixel (UG_S16 sPosX, UG_S16 sPosY, UG_COLOR Color)
{
	assert (s_pThis != 0);
	assert (s_pThis->m_pScreen != 0);
	s_pThis->m_pScreen->SetPixel ((unsigned) sPosX, (unsigned) sPosY, (TScreenColor) Color);
}

void CUGUI::MouseEventHandler (TMouseEvent Event, unsigned nButtons,
			       unsigned nPosX, unsigned nPosY, int nWheelMove)
{
	switch (Event)
	{
	case MouseEventMouseDown:
		if (nButtons & MOUSE_BUTTON_LEFT)
		{
			UG_TouchUpdate ((UG_S16) nPosX, (UG_S16) nPosY, TOUCH_STATE_PRESSED);
		}
		break;

	case MouseEventMouseUp:
		if (nButtons & MOUSE_BUTTON_LEFT)
		{
			UG_TouchUpdate (-1, -1, TOUCH_STATE_RELEASED);
		}
		break;

	default:
		break;
	}
}

void CUGUI::MouseEventStub (TMouseEvent Event, unsigned nButtons,
			    unsigned nPosX, unsigned nPosY, int nWheelMove)
{
	assert (s_pThis != 0);
	s_pThis->MouseEventHandler (Event, nButtons, nPosX, nPosY, nWheelMove);
}

void CUGUI::TouchScreenEventHandler (TTouchScreenEvent Event, unsigned nID, unsigned nPosX, unsigned nPosY)
{
	switch (Event)
	{
	case TouchScreenEventFingerDown:
		if (nID == 0)
		{
			UG_TouchUpdate ((UG_S16) nPosX, (UG_S16) nPosY, TOUCH_STATE_PRESSED);
		}
		break;

	case TouchScreenEventFingerUp:
		if (nID == 0)
		{
			UG_TouchUpdate (-1, -1, TOUCH_STATE_RELEASED);
		}
		break;

	default:
		break;
	}
}

void CUGUI::TouchScreenEventStub (TTouchScreenEvent Event, unsigned nID, unsigned nPosX, unsigned nPosY)
{
	assert (s_pThis != 0);
	s_pThis->TouchScreenEventHandler (Event, nID, nPosX, nPosY);
}

void CUGUI::MouseRemovedHandler (CDevice *pDevice, void *pContext)
{
	assert (s_pThis != 0);
	s_pThis->m_pMouseDevice = 0;
}
