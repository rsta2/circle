//
// uguicpp.cpp
//
#include <ugui/uguicpp.h>
#include <circle/devicenameservice.h>
#include <circle/timer.h>
#include <assert.h>

CUGUI *CUGUI::s_pThis = 0;

CUGUI::CUGUI (CScreenDevice *pScreen)
:	m_pScreen (pScreen),
	m_pTouchScreen (0),
	m_nLastUpdate (0)
{
	assert (s_pThis == 0);
	s_pThis = this;
}

CUGUI::~CUGUI (void)
{
	m_pTouchScreen = 0;
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

	CUSBMouseDevice *pMouse =
		(CUSBMouseDevice *) CDeviceNameService::Get ()->GetDevice ("umouse1", FALSE);
	if (pMouse != 0)
	{
		if (pMouse->Setup (m_pScreen->GetWidth (), m_pScreen->GetHeight ()))
		{
			pMouse->ShowCursor (TRUE);

			pMouse->RegisterEventHandler (MouseEventStub);
		}
	}

	m_pTouchScreen = (CTouchScreenDevice *) CDeviceNameService::Get ()->GetDevice ("touch1", FALSE);
	if (m_pTouchScreen != 0)
	{
		m_pTouchScreen->RegisterEventHandler (TouchScreenEventStub);
	}

	UG_TouchUpdate (-1, -1, TOUCH_STATE_RELEASED);

	return TRUE;
}

void CUGUI::Update (void)
{
	UG_Update ();

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

void CUGUI::MouseEventHandler (TMouseEvent Event, unsigned nButtons, unsigned nPosX, unsigned nPosY)
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

void CUGUI::MouseEventStub (TMouseEvent Event, unsigned nButtons, unsigned nPosX, unsigned nPosY)
{
	assert (s_pThis != 0);
	s_pThis->MouseEventHandler (Event, nButtons, nPosX, nPosY);
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
