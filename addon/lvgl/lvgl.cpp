//
// lvgl.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2019-2020  R. Stange <rsta2@o2online.de>
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
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/string.h>
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

	lv_init ();

	lv_log_register_print_cb (LogPrint);

	m_pBuffer1 = new (HEAP_DMA30) lv_color_t[LV_HOR_RES_MAX*10];
	m_pBuffer2 = new (HEAP_DMA30) lv_color_t[LV_HOR_RES_MAX*10];
	if (   m_pBuffer1 == 0
	    || m_pBuffer2 == 0)
	{
		return FALSE;
	}

	static lv_disp_buf_t disp_buf;
	lv_disp_buf_init (&disp_buf, m_pBuffer1, m_pBuffer2, LV_HOR_RES_MAX*10);

	lv_disp_drv_t disp_drv;
	lv_disp_drv_init (&disp_drv);
	disp_drv.buffer = &disp_buf;
	disp_drv.flush_cb = DisplayFlush;
	lv_disp_drv_register (&disp_drv);

	m_pMouseDevice = (CMouseDevice *) CDeviceNameService::Get ()->GetDevice ("mouse1", FALSE);
	if (m_pMouseDevice != 0)
	{
		if (m_pMouseDevice->Setup (m_pFrameBuffer->GetWidth (), m_pFrameBuffer->GetHeight ()))
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
			m_pTouchScreen->RegisterEventHandler (TouchScreenEventHandler);
		}
	}

	lv_indev_drv_t indev_drv;
	lv_indev_drv_init (&indev_drv);
	indev_drv.type = LV_INDEV_TYPE_POINTER;
	indev_drv.read_cb = PointerRead;
	lv_indev_drv_register (&indev_drv);

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
			}
		}
	}

	lv_task_handler ();

	unsigned nTicks = CTimer::Get ()->GetClockTicks ();
	if (nTicks - m_nLastUpdate >= CLOCKHZ/1000)
	{
		lv_tick_inc ((nTicks - m_nLastUpdate) / (CLOCKHZ/1000));

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

void CLVGL::DisplayFlush (lv_disp_drv_t *pDriver, const lv_area_t *pArea, lv_color_t *pBuffer)
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

	assert (pDriver != 0);
	s_pThis->m_DMAChannel.SetCompletionRoutine (DisplayFlushComplete, pDriver);
	s_pThis->m_DMAChannel.Start ();
}

void CLVGL::DisplayFlushComplete (unsigned nChannel, boolean bStatus, void *pParam)
{
	assert (bStatus);

	lv_disp_drv_t *pDriver = (lv_disp_drv_t *) pParam;
	assert (pDriver != 0);

	lv_disp_flush_ready (pDriver);
}

bool CLVGL::PointerRead (lv_indev_drv_t *pDriver, lv_indev_data_t *pData)
{
	assert (s_pThis != 0);

	pData->state = s_pThis->m_PointerData.state;
	pData->point.x = s_pThis->m_PointerData.point.x;
	pData->point.y = s_pThis->m_PointerData.point.y;

	return false;
}

void CLVGL::MouseEventHandler (TMouseEvent Event, unsigned nButtons,
			       unsigned nPosX, unsigned nPosY, int nWheelMove)
{
	assert (s_pThis != 0);

	switch (Event)
	{
	case MouseEventMouseDown:
	case MouseEventMouseMove:
		if (nButtons & MOUSE_BUTTON_LEFT)
		{
			s_pThis->m_PointerData.state = LV_INDEV_STATE_PR;
			s_pThis->m_PointerData.point.x = nPosX;
			s_pThis->m_PointerData.point.y = nPosY;
		}
		break;

	case MouseEventMouseUp:
		if (nButtons & MOUSE_BUTTON_LEFT)
		{
			s_pThis->m_PointerData.state = LV_INDEV_STATE_REL;
		}
		break;

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
			s_pThis->m_PointerData.state = LV_INDEV_STATE_PR;
			s_pThis->m_PointerData.point.x = nPosX;
			s_pThis->m_PointerData.point.y = nPosY;
		}
		break;

	case TouchScreenEventFingerUp:
		if (nID == 0)
		{
			s_pThis->m_PointerData.state = LV_INDEV_STATE_REL;
		}
		break;

	default:
		break;
	}
}

void CLVGL::LogPrint (lv_log_level_t Level, const char *pFile, uint32_t nLine,
		      const char *pFunction, const char *pDescription)
{
	TLogSeverity Severity;
	const char *pPrefix;

	switch (Level)
	{
	default:
	case LV_LOG_LEVEL_ERROR:	Severity = LogError;	pPrefix = "ERROR";	break;
	case LV_LOG_LEVEL_WARN:		Severity = LogWarning;	pPrefix = "WARNING";	break;
	case LV_LOG_LEVEL_INFO:		Severity = LogNotice;	pPrefix = "INFO";	break;
	case LV_LOG_LEVEL_TRACE:	Severity = LogDebug;	pPrefix = "TRACE";	break;
	}

	CString Source;
	Source.Format ("%s(%d)", pFile, nLine);

	CLogger::Get ()->Write (Source, Severity, "%s: %s: %s", pPrefix, pFunction, pDescription);
}

void CLVGL::MouseRemovedHandler (CDevice *pDevice, void *pContext)
{
	assert (s_pThis != 0);
	s_pThis->m_pMouseDevice = 0;
}
