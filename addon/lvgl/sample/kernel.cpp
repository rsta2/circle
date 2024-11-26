//
// kernel.cpp
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
#include "kernel.h"
#include "../lvgl/demos/lv_demos.h"

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_USBHCI (&m_Interrupt, &m_Timer, TRUE),
#ifdef SPI_DISPLAY
	m_SPIMaster (SPI_CLOCK_SPEED, SPI_CPOL, SPI_CPHA, SPI_MASTER_DEVICE),
	m_SPIDisplay (&m_SPIMaster, DISPLAY_PARAMETERS, FALSE),	// FALSE: use RGB565, not RGB565_BE
	m_GUI (&m_SPIDisplay)
#elif defined (I2C_DISPLAY)
	m_I2CMaster (I2C_MASTER_DEVICE, TRUE),			// TRUE: I2C fast mode
	m_I2CDisplay (&m_I2CMaster, DISPLAY_PARAMETERS),
	m_GUI (&m_I2CDisplay)
#else
	m_GUI (&m_Screen)
#endif
{
	m_ActLED.Blink (5);	// show we are alive
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;

	if (bOK)
	{
		bOK = m_Screen.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Serial.Initialize (115200);
	}

	if (bOK)
	{
		CDevice *pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
		if (pTarget == 0)
		{
			pTarget = &m_Screen;
		}

		bOK = m_Logger.Initialize (pTarget);
	}

	if (bOK)
	{
		bOK = m_Interrupt.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Timer.Initialize ();
	}

	if (bOK)
	{
		bOK = m_USBHCI.Initialize ();
	}

#ifdef SPI_DISPLAY
	if (bOK)
	{
		bOK = m_SPIMaster.Initialize ();
	}

	if (bOK)
	{
#ifdef DISPLAY_ROTATION
		m_SPIDisplay.SetRotation (DISPLAY_ROTATION);
#endif

		bOK = m_SPIDisplay.Initialize ();
	}
#elif defined (I2C_DISPLAY)
	if (bOK)
	{
		bOK = m_I2CMaster.Initialize ();
	}

	if (bOK)
	{
#ifdef DISPLAY_ROTATION
		m_I2CDisplay.SetRotation (DISPLAY_ROTATION);
#endif

		bOK = m_I2CDisplay.Initialize ();
	}
#else
	if (bOK)
	{
		m_RPiTouchScreen.Initialize ();
	}
#endif

	if (bOK)
	{
		bOK = m_GUI.Initialize ();
	}

	return bOK;
}

#ifdef I2C_DISPLAY

static void event_handler (lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code (e);

	if (code == LV_EVENT_CLICKED)
	{
		LV_LOG_USER ("Clicked");
	}
}

static void lv_example_button (void)
{
	lv_obj_t *btn = lv_button_create (lv_screen_active ());
	lv_obj_add_event_cb (btn, event_handler, LV_EVENT_ALL, NULL);
	lv_obj_align (btn, LV_ALIGN_CENTER, 0, 0);
	lv_obj_remove_flag (btn, LV_OBJ_FLAG_PRESS_LOCK);

	lv_obj_t *label = lv_label_create (btn);
	lv_obj_set_style_text_font (label, &lv_font_montserrat_20, LV_PART_MAIN);
	lv_label_set_text (label, "Button");
	lv_obj_center (label);
}

#endif

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

#ifdef I2C_DISPLAY
	lv_example_button ();
#else
	lv_demo_widgets ();
#endif

	while (1)
	{
		boolean bUpdated = m_USBHCI.UpdatePlugAndPlay ();

		m_GUI.Update (bUpdated);
	}

	return ShutdownHalt;
}
