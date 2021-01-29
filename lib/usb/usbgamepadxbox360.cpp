//
// usbgamepadxbox360.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2018-2021  R. Stange <rsta2@o2online.de>
//
// Information to implement this was taken from:
//	https://github.com/felis/USB_Host_Shield_2.0/blob/master/XBOXUSB.cpp
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
#include <circle/usb/usbgamepadxbox360.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/synchronize.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <circle/macros.h>
#include <circle/debug.h>
#include <assert.h>

struct TXbox360Report
{
	u16	Header;
#define REPORT_HEADER			0x1400

	u16	Buttons;
#define REPORT_BUTTONS			16

#define REPORT_ANALOG_BUTTONS		2
	u8	AnalogButton[REPORT_ANALOG_BUTTONS];
#define REPORT_ANALOG_BUTTON_MINIMUM	0
#define REPORT_ANALOG_BUTTON_THRESHOLD	128
#define REPORT_ANALOG_BUTTON_MAXIMUM	255

#define REPORT_AXES			4
	s16	Axes[REPORT_AXES];
#define REPORT_AXES_MINIMUM		(-32768)
#define REPORT_AXES_MAXIMUM		32767
}
PACKED;

#define REPORT_SIZE	sizeof (TXbox360Report)

static const char FromUSBPadXbox360[] = "usbpadxbox360";

CUSBGamePadXbox360Device::CUSBGamePadXbox360Device (CUSBFunction *pFunction)
:	CUSBGamePadDevice (pFunction)
{
}

CUSBGamePadXbox360Device::~CUSBGamePadXbox360Device (void)
{
}

boolean CUSBGamePadXbox360Device::Configure (void)
{
	m_usReportSize = REPORT_SIZE;

	if (!CUSBGamePadDevice::Configure ())
	{
		CLogger::Get ()->Write (FromUSBPadXbox360, LogError, "Cannot configure gamepad device");

		return FALSE;
	}

	m_State.nbuttons = GAMEPAD_BUTTONS_STANDARD;
	m_State.naxes = REPORT_AXES+REPORT_ANALOG_BUTTONS;
	for (unsigned i = 0; i < REPORT_AXES; i++)
	{
		m_State.axes[i].minimum = GAMEPAD_AXIS_DEFAULT_MINIMUM;
		m_State.axes[i].maximum = GAMEPAD_AXIS_DEFAULT_MAXIMUM;
	}
	for (unsigned i = 0; i < REPORT_ANALOG_BUTTONS; i++)
	{
		m_State.axes[REPORT_AXES+i].minimum = REPORT_ANALOG_BUTTON_MINIMUM;
		m_State.axes[REPORT_AXES+i].maximum = REPORT_ANALOG_BUTTON_MAXIMUM;
	}
	m_State.nhats = 0;

	if (!SetLEDMode ((TGamePadLEDMode) m_nDeviceNumber))
	{
		return FALSE;
	}

	return StartRequest ();
}

void CUSBGamePadXbox360Device::ReportHandler (const u8 *pReport, unsigned nReportSize)
{
	if (   pReport != 0
	    && nReportSize == REPORT_SIZE
	    && pReport[0] == (REPORT_HEADER & 0xFF)
	    && pReport[1] == (REPORT_HEADER >> 8)
	    && m_pStatusHandler != 0)
	{
		//debug_hexdump (pReport, m_usReportSize, FromUSBPadXbox360);

		DecodeReport (pReport);

		(*m_pStatusHandler) (m_nDeviceNumber-1, &m_State);
	}
}

void CUSBGamePadXbox360Device::DecodeReport (const u8 *pReportBuffer)
{
	const TXbox360Report *pReport = reinterpret_cast<const TXbox360Report *> (pReportBuffer);
	assert (pReport != 0);
	assert (pReport->Header == REPORT_HEADER);

	static const u32 ButtonMap[] =
	{
		GamePadButtonUp,
		GamePadButtonDown,
		GamePadButtonLeft,
		GamePadButtonRight,
		GamePadButtonStart,
		GamePadButtonBack,
		GamePadButtonL3,
		GamePadButtonR3,
		GamePadButtonLB,
		GamePadButtonRB,
		GamePadButtonXbox,
		0,
		GamePadButtonA,
		GamePadButtonB,
		GamePadButtonX,
		GamePadButtonY
	};

	u32 nButtons = pReport->Buttons;
	m_State.buttons = 0;
	for (unsigned i = 0; i < REPORT_BUTTONS; i++)
	{
		if (nButtons & 1)
		{
			m_State.buttons |= ButtonMap[i];
		}

		nButtons >>= 1;
	}

	static const unsigned AxisMap[] =
	{
		GamePadAxisLeftX,
		GamePadAxisLeftY,
		GamePadAxisRightX,
		GamePadAxisRightY,
		GamePadAxisButtonLT,
		GamePadAxisButtonRT
	};

	for (unsigned i = 0; i < REPORT_AXES; i++)
	{
		int nValue = pReport->Axes[i];

		// remap axis value to default range [0, 255]
		nValue = (unsigned) (nValue - REPORT_AXES_MINIMUM) >> 8;

		unsigned nAxis = AxisMap[i];
		if (   nAxis == GamePadAxisLeftY
		    || nAxis == GamePadAxisRightY)	// Y-axes have to be reversed
		{
			nValue = GAMEPAD_AXIS_DEFAULT_MAXIMUM - nValue;
		}

		m_State.axes[nAxis].value = nValue;
	}

	for (unsigned i = 0; i < REPORT_ANALOG_BUTTONS; i++)
	{
		m_State.axes[AxisMap[REPORT_AXES+i]].value = pReport->AnalogButton[i];

		if (pReport->AnalogButton[i] >= REPORT_ANALOG_BUTTON_THRESHOLD)
		{
			m_State.buttons |= GamePadButtonLT << i;
		}
	}
}

boolean CUSBGamePadXbox360Device::SetLEDMode (TGamePadLEDMode Mode)
{
	static const u8 LEDMode[] = {0x00, 0x06, 0x07, 0x08, 0x09};

	if (Mode >= sizeof LEDMode / sizeof LEDMode[0])
	{
		return FALSE;
	}

	DMA_BUFFER (u8, Command, 3);
	Command[0] = 0x01;
	Command[1] = 0x03;
	Command[2] = LEDMode[Mode];

	return SendToEndpointOut (Command, 3);
}

boolean CUSBGamePadXbox360Device::SetRumbleMode (TGamePadRumbleMode Mode)
{
	DMA_BUFFER (u8, Command, 8);
	memset (Command, 0, 8);
	Command[1] = 0x08;

	switch (Mode)
	{
	case GamePadRumbleModeOff:
		break;

	case GamePadRumbleModeLow:
		Command[4] = 0xFF;
		break;

	case GamePadRumbleModeHigh:
		Command[3] = 0xFF;
		break;

	default:
		assert (0);
		return FALSE;
	}

	return SendToEndpointOut (Command, 8);
}
