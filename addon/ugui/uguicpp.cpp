//
// uguicpp.cpp
//
#include <ugui/uguicpp.h>
#include <circle/devicenameservice.h>
#include <assert.h>

CUGUI *CUGUI::s_pThis = 0;

CUGUI::CUGUI (CScreenDevice *pScreen)
:	m_pScreen (pScreen)
{
	assert (s_pThis == 0);
	s_pThis = this;
}

CUGUI::~CUGUI (void)
{
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

	UG_TouchUpdate (-1, -1, TOUCH_STATE_RELEASED);

	return TRUE;
}

void CUGUI::Update (void)
{
	UG_Update ();
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
