//
// lvgl.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2019-2024  R. Stange <rsta2@o2online.de>
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
#include <lvgl/lvgl.h>
#include <circle/devicenameservice.h>
#include <circle/koptions.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/string.h>
#include <circle/util.h>
#include <circle/new.h>

CLVGL *CLVGL::s_pThis = 0;

CLVGL::CLVGL (CScreenDevice *pScreen)
:	m_pBuffer1 (0),
	m_pBuffer2 (0),
	m_pScreen (pScreen),
	m_pDisplay (0),
	m_nLastUpdate (0),
	m_pMouseDevice (0),
	m_pTouchScreen (0),
	m_nLastTouchUpdate (0),
	m_pIndev (0),
	m_pCursorDesc (0)
{
	assert (s_pThis == 0);
	s_pThis = this;

	m_PointerData.state = LV_INDEV_STATE_REL;
	m_PointerData.point.x = 0;
	m_PointerData.point.y = 0;
}

CLVGL::CLVGL (CDisplay *pDisplay)
:	m_pBuffer1 (0),
	m_pBuffer2 (0),
	m_pScreen (0),
	m_pDisplay (pDisplay),
	m_nLastUpdate (0),
	m_pMouseDevice (0),
	m_pTouchScreen (0),
	m_nLastTouchUpdate (0),
	m_pIndev (0),
	m_pCursorDesc (0)
{
	assert (s_pThis == 0);
	s_pThis = this;

	m_PointerData.state = LV_INDEV_STATE_REL;
	m_PointerData.point.x = 0;
	m_PointerData.point.y = 0;
}

CLVGL::~CLVGL (void)
{
	s_pThis = 0;

	delete m_pCursorDesc;

	m_pTouchScreen = 0;
	m_pMouseDevice = 0;
	m_pDisplay = 0;
	m_pScreen = 0;

	delete [] m_pBuffer1;
	delete [] m_pBuffer2;
	m_pBuffer1 = 0;
	m_pBuffer2 = 0;
}

boolean CLVGL::Initialize (void)
{
	if (m_pDisplay == 0)
	{
		assert (m_pScreen != 0);
		m_pDisplay = m_pScreen->GetFrameBuffer ();
	}

	assert (m_pDisplay != 0);
	size_t nWidth = m_pDisplay->GetWidth ();
	size_t nHeight = m_pDisplay->GetHeight ();

	lv_init ();

	lv_log_register_print_cb (LogPrint);

	unsigned nBufSizePixels = nWidth * nHeight/10;

	m_pBuffer1 = new (HEAP_DMA30) u16[nBufSizePixels];
	m_pBuffer2 = new (HEAP_DMA30) u16[nBufSizePixels];
	if (   m_pBuffer1 == 0
	    || m_pBuffer2 == 0)
	{
		return FALSE;
	}

	static lv_display_t *display = lv_display_create (nWidth, nHeight);

	if (m_pDisplay->GetDepth () == 1)
	{
		lv_display_set_color_format (display, LV_COLOR_FORMAT_I1);
	}
	else
	{
		assert (m_pDisplay->GetDepth () == LV_COLOR_DEPTH);
	}

	lv_display_set_flush_cb (display, DisplayFlush);
	lv_display_set_buffers (display, m_pBuffer1, m_pBuffer2, nBufSizePixels * sizeof (u16),
				LV_DISP_RENDER_MODE_PARTIAL);

	m_pMouseDevice = (CMouseDevice *) CDeviceNameService::Get ()->GetDevice ("mouse1", FALSE);
	if (m_pMouseDevice != 0)
	{
		if (m_pMouseDevice->Setup (m_pDisplay, FALSE))
		{
			m_PointerData.point.x = (nWidth+1) / 2;
			m_PointerData.point.y = (nHeight+1) / 2;

			m_pMouseDevice->ShowCursor (TRUE);

			m_pMouseDevice->RegisterEventHandler (MouseEventHandler);

			m_pMouseDevice->RegisterRemovedHandler (MouseRemovedHandler);
		}
		else
		{
			m_pMouseDevice = 0;
		}
	}

	if (m_pMouseDevice == 0)
	{
		m_pTouchScreen = (CTouchScreenDevice *) CDeviceNameService::Get ()->GetDevice ("touch1", FALSE);
		if (m_pTouchScreen != 0)
		{
			m_pTouchScreen->Setup (m_pDisplay);

			const unsigned *pCalibration = CKernelOptions::Get ()->GetTouchScreen ();
			if (pCalibration != 0)
			{
				m_pTouchScreen->SetCalibration (pCalibration);
			}

			m_pTouchScreen->RegisterEventHandler (TouchScreenEventHandler);
		}
	}

	static lv_indev_t *indev = lv_indev_create ();
	lv_indev_set_type (indev, LV_INDEV_TYPE_POINTER);
	lv_indev_set_read_cb (indev, PointerRead);

	assert (m_pIndev == 0);
	m_pIndev = indev;

	if (m_pMouseDevice != 0)
	{
		assert (m_pIndev != 0);
		SetupCursor (m_pIndev);
	}

	CTimer::Get ()->RegisterPeriodicHandler (PeriodicTickHandler);

	return TRUE;
}

void CLVGL::Update (boolean bPlugAndPlayUpdated)
{
	if (   bPlugAndPlayUpdated
	    && m_pMouseDevice == 0)
	{
		m_pMouseDevice =
			(CMouseDevice *) CDeviceNameService::Get ()->GetDevice ("mouse1", FALSE);
		if (m_pMouseDevice != 0)
		{
			assert (m_pDisplay != 0);
			if (m_pMouseDevice->Setup (m_pDisplay, FALSE))
			{
				m_PointerData.point.x = (m_pDisplay->GetWidth ()+1) / 2;
				m_PointerData.point.y = (m_pDisplay->GetHeight ()+1) / 2;

				m_pMouseDevice->ShowCursor (TRUE);

				m_pMouseDevice->RegisterEventHandler (MouseEventHandler);

				m_pMouseDevice->RegisterRemovedHandler (MouseRemovedHandler);

				assert (m_pIndev != 0);
				SetupCursor (m_pIndev);
			}
		}
	}

	unsigned nTicks = CTimer::Get ()->GetClockTicks ();
	if (nTicks - m_nLastUpdate >= 5*CLOCKHZ/1000)
	{
		lv_timer_handler ();

		m_nLastUpdate = nTicks;
	}

	if (m_pMouseDevice != 0)
	{
		m_pMouseDevice->UpdateCursor ();
	}
	else if (m_pTouchScreen != 0)
	{
		if (nTicks - m_nLastTouchUpdate >= CLOCKHZ/60)
		{
			m_pTouchScreen->Update ();

			m_nLastTouchUpdate = nTicks;
		}
	}
}

void CLVGL::DisplayFlush (lv_display_t *pDisplay, const lv_area_t *pArea, u8 *pBuffer)
{
	assert (s_pThis != 0);

	assert (pArea != 0);
	CDisplay::TArea Area;
	Area.x1 = pArea->x1;
	Area.x2 = pArea->x2;
	Area.y1 = pArea->y1;
	Area.y2 = pArea->y2;

	assert (Area.x1 <= Area.x2);
	assert (Area.y1 <= Area.y2);
	assert (pBuffer != 0);

	assert (s_pThis->m_pDisplay != 0);
	if (s_pThis->m_pDisplay->GetDepth () == 1)
	{
		pBuffer += 8;		// ignore palette
	}

	assert (pDisplay != 0);
	s_pThis->m_pDisplay->SetArea (Area, pBuffer, DisplayFlushComplete, pDisplay);
}

void CLVGL::DisplayFlushComplete (void *pParam)
{
	lv_display_t *pDisplay = (lv_display_t *) pParam;
	assert (pDisplay != 0);

	lv_display_flush_ready (pDisplay);
}

void CLVGL::PointerRead (lv_indev_t *pIndev, lv_indev_data_t *pData)
{
	assert (s_pThis != 0);

	pData->state = s_pThis->m_PointerData.state;
	pData->point.x = s_pThis->m_PointerData.point.x;
	pData->point.y = s_pThis->m_PointerData.point.y;
}

void CLVGL::MouseEventHandler (TMouseEvent Event, unsigned nButtons,
			       unsigned nPosX, unsigned nPosY, int nWheelMove)
{
	assert (s_pThis != 0);

	s_pThis->m_PointerData.point.x = nPosX;
	s_pThis->m_PointerData.point.y = nPosY;

	switch (Event)
	{
	case MouseEventMouseDown:
		if (nButtons & MOUSE_BUTTON_LEFT)
		{
			s_pThis->m_PointerData.state = LV_INDEV_STATE_PRESSED;
		}
		break;

	case MouseEventMouseUp:
		if (nButtons & MOUSE_BUTTON_LEFT)
		{
			s_pThis->m_PointerData.state = LV_INDEV_STATE_RELEASED;
		}
		break;

	case MouseEventMouseMove:
	default:
		break;
	}
}

void CLVGL::TouchScreenEventHandler (TTouchScreenEvent Event, unsigned nID,
				     unsigned nPosX, unsigned nPosY)
{
	assert (s_pThis != 0);
	assert (s_pThis->m_pDisplay != 0);

	switch (Event)
	{
	case TouchScreenEventFingerDown:
	case TouchScreenEventFingerMove:
		if (   nID == 0
		    && nPosX < s_pThis->m_pDisplay->GetWidth ()
		    && nPosY < s_pThis->m_pDisplay->GetHeight ())
		{
			s_pThis->m_PointerData.state = LV_INDEV_STATE_PRESSED;
			s_pThis->m_PointerData.point.x = nPosX;
			s_pThis->m_PointerData.point.y = nPosY;
		}
		break;

	case TouchScreenEventFingerUp:
		if (nID == 0)
		{
			s_pThis->m_PointerData.state = LV_INDEV_STATE_RELEASED;
		}
		break;

	default:
		break;
	}
}

void CLVGL::LogPrint (lv_log_level_t LogLevel, const char *pMessage)
{
	assert (pMessage != 0);
	size_t nLen = strlen (pMessage);
	assert (nLen > 0);

	char Buffer[nLen+1];
	strcpy (Buffer, pMessage);

	if (Buffer[nLen-1] == '\n')
	{
		Buffer[nLen-1] = '\0';
	}

	TLogSeverity Severity;
	switch (LogLevel)
	{
	case LV_LOG_LEVEL_ERROR:
		Severity = LogError;
		break;

	case LV_LOG_LEVEL_WARN:
		Severity = LogWarning;
		break;

	case LV_LOG_LEVEL_INFO:
	case LV_LOG_LEVEL_USER:
		Severity = LogNotice;
		break;

	default:
		Severity = LogDebug;
		break;
	}

	CLogger::Get ()->Write ("lvgl", Severity, Buffer);
}

void CLVGL::MouseRemovedHandler (CDevice *pDevice, void *pContext)
{
	assert (s_pThis != 0);
	s_pThis->m_pMouseDevice = 0;
}

void CLVGL::PeriodicTickHandler (void)
{
	lv_tick_inc (1000 / HZ);
}

#define CURSOR_WIDTH	11
#define CURSOR_HEIGHT	16

static const u8 CursorSymbol[CURSOR_HEIGHT * CURSOR_WIDTH * 4] =
{
#define B	0x00, 0x00, 0x00, 0x00
#define G	0x80, 0x80, 0x80, 0xFF
#define W	0xFF, 0xFF, 0xFF, 0xFF
	G,G,B,B,B,B,B,B,B,B,B,
	G,W,G,B,B,B,B,B,B,B,B,
	G,W,W,G,B,B,B,B,B,B,B,
	G,W,W,W,G,B,B,B,B,B,B,
	G,W,W,W,W,G,B,B,B,B,B,
	G,W,W,W,W,W,G,B,B,B,B,
	G,W,W,W,W,W,W,G,B,B,B,
	G,W,W,W,W,W,W,W,G,B,B,
	G,W,W,W,W,W,W,W,W,G,B,
	G,G,G,G,W,W,G,G,G,G,G,
	B,B,B,G,W,W,W,G,B,B,B,
	B,B,B,B,G,W,W,G,B,B,B,
	B,B,B,B,G,W,W,W,G,B,B,
	B,B,B,B,B,G,W,W,G,B,B,
	B,B,B,B,B,G,W,W,W,G,B,
	B,B,B,B,B,B,G,G,G,G,B,
};

void CLVGL::SetupCursor (lv_indev_t *pIndev)
{
	if (m_pCursorDesc == 0)
	{
		m_pCursorDesc = new lv_image_dsc_t;
		assert (m_pCursorDesc);
		memset (m_pCursorDesc, 0, sizeof *m_pCursorDesc);

		m_pCursorDesc->header.magic = LV_IMAGE_HEADER_MAGIC;
		m_pCursorDesc->header.h = CURSOR_HEIGHT;
		m_pCursorDesc->header.w = CURSOR_WIDTH;
		m_pCursorDesc->header.cf = LV_COLOR_FORMAT_ARGB8888;
		m_pCursorDesc->header.stride = CURSOR_WIDTH * 4;
		m_pCursorDesc->data_size = sizeof CursorSymbol;
		m_pCursorDesc->data = CursorSymbol;

		lv_obj_t *pCursorImage = lv_image_create (lv_screen_active ());
		assert (pCursorImage != 0);
		lv_image_set_src (pCursorImage, m_pCursorDesc);

		assert (pIndev != 0);
		lv_indev_set_cursor (pIndev, pCursorImage);
	}
}
