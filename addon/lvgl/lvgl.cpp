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

CLVGL::CLVGL (CScreenDevice *pScreen, CInterruptSystem *pInterrupt)
:	m_pBuffer1 (0),
	m_pBuffer2 (0),
	m_pScreen (pScreen),
	m_pFrameBuffer (0),
	m_DMAChannel (DMA_CHANNEL_NORMAL, pInterrupt),
	m_nLastUpdate (0),
	m_pMouseDevice (0),
	m_pTouchScreen (0),
	m_nLastTouchUpdate (0)
#if RASPPI >= 5
	, m_pIndev (0),
	m_pCursorDesc (0)
#endif
{
	assert (s_pThis == 0);
	s_pThis = this;

	m_PointerData.state = LV_INDEV_STATE_REL;
	m_PointerData.point.x = 0;
	m_PointerData.point.y = 0;
}

CLVGL::CLVGL (CBcmFrameBuffer *pFrameBuffer, CInterruptSystem *pInterrupt)
:	m_pBuffer1 (0),
	m_pBuffer2 (0),
	m_pScreen (0),
	m_pFrameBuffer (pFrameBuffer),
	m_DMAChannel (DMA_CHANNEL_NORMAL, pInterrupt),
	m_nLastUpdate (0),
	m_pMouseDevice (0),
	m_pTouchScreen (0),
	m_nLastTouchUpdate (0)
#if RASPPI >= 5
	, m_pIndev (0),
	m_pCursorDesc (0)
#endif
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

#if RASPPI >= 5
	delete m_pCursorDesc;
#endif

	m_pTouchScreen = 0;
	m_pMouseDevice = 0;
	m_pFrameBuffer = 0;
	m_pScreen = 0;

	delete [] m_pBuffer1;
	delete [] m_pBuffer2;
	m_pBuffer1 = 0;
	m_pBuffer2 = 0;
}

boolean CLVGL::Initialize (void)
{
	if (m_pFrameBuffer == 0)
	{
		assert (m_pScreen != 0);
		m_pFrameBuffer = m_pScreen->GetFrameBuffer ();
	}

	assert (m_pFrameBuffer != 0);
	assert (m_pFrameBuffer->GetDepth () == LV_COLOR_DEPTH);
	size_t nWidth = m_pFrameBuffer->GetWidth ();
	size_t nHeight = m_pFrameBuffer->GetHeight ();

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
	lv_display_set_flush_cb (display, DisplayFlush);
	lv_display_set_buffers (display, m_pBuffer1, m_pBuffer2, nBufSizePixels * sizeof (u16),
				LV_DISP_RENDER_MODE_PARTIAL);

	m_pMouseDevice = (CMouseDevice *) CDeviceNameService::Get ()->GetDevice ("mouse1", FALSE);
	if (m_pMouseDevice != 0)
	{
		if (m_pMouseDevice->Setup (nWidth, nHeight))
		{
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
			const unsigned *pCalibration = CKernelOptions::Get ()->GetTouchScreen ();
			if (pCalibration != 0)
			{
				m_pTouchScreen->SetCalibration (pCalibration, nWidth, nHeight);
			}

			m_pTouchScreen->RegisterEventHandler (TouchScreenEventHandler);
		}
	}

	static lv_indev_t *indev = lv_indev_create ();
	lv_indev_set_type (indev, LV_INDEV_TYPE_POINTER);
	lv_indev_set_read_cb (indev, PointerRead);
#if RASPPI >= 5
	assert (m_pIndev == 0);
	m_pIndev = indev;
#endif

#if RASPPI >= 5
	if (m_pMouseDevice != 0)
	{
		assert (m_pIndev != 0);
		SetupCursor (m_pIndev);
	}
#endif

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
			assert (m_pFrameBuffer != 0);
			if (m_pMouseDevice->Setup (m_pFrameBuffer->GetWidth (),
						   m_pFrameBuffer->GetHeight ()))
			{
				m_pMouseDevice->ShowCursor (TRUE);

				m_pMouseDevice->RegisterEventHandler (MouseEventHandler);

				m_pMouseDevice->RegisterRemovedHandler (MouseRemovedHandler);

#if RASPPI >= 5
				assert (m_pIndev != 0);
				SetupCursor (m_pIndev);
#endif
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
	int32_t x1 = pArea->x1;
	int32_t x2 = pArea->x2;
	int32_t y1 = pArea->y1;
	int32_t y2 = pArea->y2;

	assert (x1 <= x2);
	assert (y1 <= y2);
	assert (pBuffer != 0);

	assert (s_pThis->m_pFrameBuffer != 0);
	void *pDestination = (void *) (uintptr) (  s_pThis->m_pFrameBuffer->GetBuffer ()
						 + y1*s_pThis->m_pFrameBuffer->GetPitch ()
						 + x1*LV_COLOR_DEPTH/8);

	size_t nBlockLength = (x2-x1+1) * LV_COLOR_DEPTH/8;

	s_pThis->m_DMAChannel.SetupMemCopy2D (pDestination, pBuffer,
					      nBlockLength, y2-y1+1,
					      s_pThis->m_pFrameBuffer->GetPitch ()-nBlockLength);

	assert (pDisplay != 0);
	s_pThis->m_DMAChannel.SetCompletionRoutine (DisplayFlushComplete, pDisplay);
	s_pThis->m_DMAChannel.Start ();
}

void CLVGL::DisplayFlushComplete (unsigned nChannel, unsigned nBuffer, boolean bStatus, void *pParam)
{
	assert (bStatus);

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

	switch (Event)
	{
	case TouchScreenEventFingerDown:
	case TouchScreenEventFingerMove:
		if (nID == 0)
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

#if RASPPI >= 5

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

#endif
