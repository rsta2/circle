//
// usbgamepadps3.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2018  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/usbgamepadps3.h>
#include <circle/usb/usbhid.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/logger.h>
#include <circle/macros.h>
#include <circle/debug.h>
#include <assert.h>

// Information to define this was taken from:
//  QtSixA, the Sixaxis Joystick Manager
//  Copyright 2008-10 Filipe Coelho <falktx@gmail.com>
//  Licensed under GPLv2
struct TPS3Report
{
	u8	Reserved[2];

#define REPORT_BUTTONS			19
	u32	Buttons;

#define REPORT_AXES			4
	u8	Axes[REPORT_AXES];
#define REPORT_AXES_MINIMUM		0
#define REPORT_AXES_MAXIMUM		255

	u8	Reserved2[4];

#define REPORT_ANALOG_BUTTONS		12
	u8	AnalogButton[REPORT_ANALOG_BUTTONS];
}
PACKED;

#define REPORT_SIZE_DEFAULT	49

static const char FromUSBPadPS3[] = "usbpadps3";

CUSBGamePadPS3Device::CUSBGamePadPS3Device (CUSBFunction *pFunction)
:	CUSBGamePadStandardDevice (pFunction),
	m_bInterfaceOK (SelectInterfaceByClass (3, 0, 0))
{
}

CUSBGamePadPS3Device::~CUSBGamePadPS3Device (void)
{
}

boolean CUSBGamePadPS3Device::Configure (void)
{
	if (!m_bInterfaceOK)
	{
		ConfigurationError (FromUSBPadPS3);

		return FALSE;
	}

	if (!CUSBGamePadStandardDevice::Configure ())
	{
		CLogger::Get ()->Write (FromUSBPadPS3, LogError, "Cannot configure gamepad device");

		return FALSE;
	}

	if (   m_usReportSize != REPORT_SIZE_DEFAULT
	    && m_usReportSize != REPORT_SIZE_DEFAULT+1)
	{
		CLogger::Get ()->Write (FromUSBPadPS3, LogError, "Invalid report size (%u)",
					(unsigned) m_usReportSize);

		return FALSE;
	}

	m_State.nbuttons = REPORT_BUTTONS;
	m_State.naxes = REPORT_AXES+REPORT_ANALOG_BUTTONS;
	for (unsigned i = 0; i < REPORT_AXES+REPORT_ANALOG_BUTTONS; i++)
	{
		m_State.axes[i].minimum = REPORT_AXES_MINIMUM;
		m_State.axes[i].maximum = REPORT_AXES_MAXIMUM;
	}
	m_State.nhats = 0;

	if (!PS3Enable ())
	{
		CLogger::Get ()->Write (FromUSBPadPS3, LogError, "Cannot enable gamepad device");

		return FALSE;
	}

	return TRUE;
}

void CUSBGamePadPS3Device::DecodeReport (const u8 *pReportBuffer)
{
	switch (m_usReportSize)
	{
	case 0:		// m_usReportSize not yet set
		CUSBGamePadStandardDevice::DecodeReport (pReportBuffer);
		return;

	case REPORT_SIZE_DEFAULT:
		break;

	case REPORT_SIZE_DEFAULT+1:
		pReportBuffer++;
		break;

	default:
		assert (0);
		break;
	}

	const TPS3Report *pReport = reinterpret_cast<const TPS3Report *> (pReportBuffer);
	assert (pReport != 0);

	u32 nButtons = pReport->Buttons;		// remap buttons:
	m_State.buttons  = (nButtons & 0x70000) >> 16;	// PS
	m_State.buttons |= (nButtons & 0xFF00)  >> 5;	// Ln/Rn, SYMBOLS
	m_State.buttons |= (nButtons & 0xFF)    << 11;	// START/SELECT, AXIS, UP/DOWN/LEFT/RIGHT

	for (unsigned i = 0; i < REPORT_AXES; i++)
	{
		m_State.axes[i].value = pReport->Axes[i];
	}

	static const unsigned AxisButtonMap[] =
	{
		GamePadAxisButtonUp,
		GamePadAxisButtonRight,
		GamePadAxisButtonDown,
		GamePadAxisButtonLeft,
		GamePadAxisButtonL2,
		GamePadAxisButtonR2,
		GamePadAxisButtonL1,
		GamePadAxisButtonR1,
		GamePadAxisButtonTriangle,
		GamePadAxisButtonCircle,
		GamePadAxisButtonCross,
		GamePadAxisButtonSquare
	};

	for (unsigned i = 0; i < REPORT_ANALOG_BUTTONS; i++)
	{
		m_State.axes[AxisButtonMap[i]].value = pReport->AnalogButton[i];
	}
}

// Copyright (C) 2014  M. Maccaferri <macca@maccasoft.com>
boolean CUSBGamePadPS3Device::PS3Enable (void)
{
	static u8 writeBuf[] =
	{
		0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00,
		0xFF, 0x27, 0x10, 0x00, 0x32,
		0xFF, 0x27, 0x10, 0x00, 0x32,
		0xFF, 0x27, 0x10, 0x00, 0x32,
		0xFF, 0x27, 0x10, 0x00, 0x32,
		0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00
	};

	static const u8 leds[] =
	{
		0x00, // OFF
		0x01, // LED1
		0x02, // LED2
		0x04, // LED3
		0x08  // LED4
	};

	/* Special PS3 Controller enable commands */
	static u8 Enable[] = {0x42, 0x0C, 0x00, 0x00};
	if (GetHost ()->ControlMessage (GetEndpoint0 (),
					REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_INTERFACE,
					SET_REPORT, (REPORT_TYPE_FEATURE << 8) | 0xF4,
					GetInterfaceNumber (), Enable, sizeof Enable) <= 0)
	{
		return FALSE;
	}

	/* Turn on LED */
	writeBuf[9] = (u8)(leds[m_nDeviceNumber] << 1);
	if (GetHost ()->ControlMessage (GetEndpoint0 (),
					REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_INTERFACE,
					SET_REPORT, (REPORT_TYPE_OUTPUT << 8) | 0x01,
					GetInterfaceNumber (), writeBuf, sizeof writeBuf) <= 0)
	{
		return FALSE;
	}

	return TRUE;
}
