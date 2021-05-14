//
// usbgamepadps3.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
//
// Information to implement this was taken from:
//	https://github.com/felis/USB_Host_Shield_2.0/blob/master/PS3USB.cpp
//	Copyright (C) 2012 Kristian Lauszus, TKJ Electronics. All rights reserved.
//	Licensed under GPLv2
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
#include <circle/util.h>
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

	u8	Reserved3[15];

	u16	Acceleration[3];	// big endian
	u16	GyroscopeZ;		// big endian
}
PACKED;

#define REPORT_SIZE_DEFAULT	49

const u8 CUSBGamePadPS3Device::s_CommandDefault[USB_GAMEPAD_PS3_COMMAND_LENGTH] =
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xFF, 0x27, 0x10, 0x00, 0x32, 0xFF, 0x27, 0x10, 0x00, 0x32,
	0xFF, 0x27, 0x10, 0x00, 0x32, 0xFF, 0x27, 0x10, 0x00, 0x32,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const char FromUSBPadPS3[] = "usbpadps3";

CUSBGamePadPS3Device::CUSBGamePadPS3Device (CUSBFunction *pFunction)
:	CUSBGamePadStandardDevice (pFunction, FALSE),
	m_bInterfaceOK (SelectInterfaceByClass (3, 0, 0))
{
	memcpy (m_CommandBuffer, s_CommandDefault, sizeof s_CommandDefault);
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

	m_State.nbuttons = GAMEPAD_BUTTONS_STANDARD;
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

	return StartRequest ();
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

	for (unsigned i = 0; i < 3; i++)
	{
		m_State.acceleration[i] = 511 - (int) bswap16 (pReport->Acceleration[i]);
	}

	m_State.gyroscope[2] = (int) bswap16 (pReport->GyroscopeZ) - 6;
}

// Copyright (C) 2014  M. Maccaferri <macca@maccasoft.com>
boolean CUSBGamePadPS3Device::PS3Enable (void)
{
	/* Special PS3 Controller enable commands */
	DMA_BUFFER (u8, Enable, 4) = {0x42, 0x0C, 0x00, 0x00};

	if (GetHost ()->ControlMessage (GetEndpoint0 (),
					REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_INTERFACE,
					SET_REPORT, (REPORT_TYPE_FEATURE << 8) | 0xF4,
					GetInterfaceNumber (), Enable, 4) < 0)
	{
		return FALSE;
	}

	return SetLEDMode ((TGamePadLEDMode) m_nDeviceNumber);
}

boolean CUSBGamePadPS3Device::SetLEDMode (TGamePadLEDMode Mode)
{
	static const u8 leds[] =
	{
		0x00, // OFF
		0x01, // LED1
		0x02, // LED2
		0x04, // LED3
		0x08, // LED4
		0x09, // LED5
		0x0A, // LED6
		0x0C, // LED7
		0x0D, // LED8
		0x0E, // LED9
		0x0F  // LED10
	};

	if (Mode >= sizeof leds / sizeof leds[0])
	{
		return FALSE;
	}

	m_CommandBuffer[9] = (u8)(leds[Mode] << 1);

	return GetHost ()->ControlMessage (GetEndpoint0 (),
					   REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_INTERFACE,
					   SET_REPORT, (REPORT_TYPE_OUTPUT << 8) | 0x01,
					   GetInterfaceNumber (),
					   m_CommandBuffer, USB_GAMEPAD_PS3_COMMAND_LENGTH) >= 0
					   ? TRUE : FALSE;
}

boolean CUSBGamePadPS3Device::SetRumbleMode (TGamePadRumbleMode Mode)
{
	DMA_BUFFER (u8, TempBuffer, USB_GAMEPAD_PS3_COMMAND_LENGTH);
	memcpy (TempBuffer, m_CommandBuffer, USB_GAMEPAD_PS3_COMMAND_LENGTH);

	switch (Mode)
	{
	case GamePadRumbleModeOff:
		TempBuffer[1] = 0;
		TempBuffer[2] = 0;
		TempBuffer[3] = 0;
		TempBuffer[4] = 0;
		break;

	case GamePadRumbleModeLow:
		TempBuffer[1] = 0xFE;
		TempBuffer[2] = 0xFF;
		TempBuffer[3] = 0xFE;
		TempBuffer[4] = 0;
		break;

	case GamePadRumbleModeHigh:
		TempBuffer[1] = 0xFE;
		TempBuffer[2] = 0;
		TempBuffer[3] = 0xFE;
		TempBuffer[4] = 0xFF;
		break;

	default:
		assert (0);
		return FALSE;
	}

	return GetHost ()->ControlMessage (GetEndpoint0 (),
					   REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_INTERFACE,
					   SET_REPORT, (REPORT_TYPE_OUTPUT << 8) | 0x01,
					   GetInterfaceNumber (),
					   TempBuffer, USB_GAMEPAD_PS3_COMMAND_LENGTH) >= 0
					   ? TRUE : FALSE;
}
