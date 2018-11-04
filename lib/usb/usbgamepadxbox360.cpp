//
// usbgamepadxbox360.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2018  R. Stange <rsta2@o2online.de>
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
#include <circle/logger.h>
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
#define REPORT_AXES_MINIMUM		-32768
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

	m_State.nbuttons = REPORT_BUTTONS+REPORT_ANALOG_BUTTONS;
	m_State.naxes = REPORT_AXES+REPORT_ANALOG_BUTTONS;
	for (unsigned i = 0; i < REPORT_AXES; i++)
	{
		m_State.axes[i].minimum = REPORT_AXES_MINIMUM;
		m_State.axes[i].maximum = REPORT_AXES_MAXIMUM;
	}
	for (unsigned i = 0; i < REPORT_ANALOG_BUTTONS; i++)
	{
		m_State.axes[REPORT_AXES+i].minimum = REPORT_ANALOG_BUTTON_MINIMUM;
		m_State.axes[REPORT_AXES+i].maximum = REPORT_ANALOG_BUTTON_MAXIMUM;
	}
	m_State.nhats = 0;

	return TRUE;
}

const TGamePadState *CUSBGamePadXbox360Device::GetReport (void)
{
	const static u8 ReportBuffer[REPORT_SIZE] = {0x00, 0x14};

	DecodeReport (ReportBuffer);

	return &m_State;
}

void CUSBGamePadXbox360Device::ReportHandler (const u8 *pReport)
{
	if (   pReport != 0
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
		1 << GamePadButtonUp,
		1 << GamePadButtonDown,
		1 << GamePadButtonLeft,
		1 << GamePadButtonRight,
		1 << GamePadButtonStart,
		1 << GamePadButtonBack,
		1 << GamePadButtonL3,
		1 << GamePadButtonR3,
		1 << GamePadButtonLB,
		1 << GamePadButtonRB,
		1 << GamePadButtonXbox,
		0,
		1 << GamePadButtonA,
		1 << GamePadButtonB,
		1 << GamePadButtonX,
		1 << GamePadButtonY
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

		unsigned nAxis = AxisMap[i];
		if (   nAxis == GamePadAxisLeftY
		    || nAxis == GamePadAxisRightY)	// Y-axes have to be reversed
		{
			switch (nValue)			// -REPORT_AXES_MINIMUM is out-of-range
			{
				case REPORT_AXES_MINIMUM:
				nValue = REPORT_AXES_MAXIMUM;
				break;

			case REPORT_AXES_MAXIMUM:
				nValue = REPORT_AXES_MINIMUM;
				break;

			default:
				nValue = -nValue;
				break;
			}
		}

		m_State.axes[nAxis].value = nValue;
	}

	for (unsigned i = 0; i < REPORT_ANALOG_BUTTONS; i++)
	{
		m_State.axes[AxisMap[REPORT_AXES+i]].value = pReport->AnalogButton[i];

		if (pReport->AnalogButton[i] >= REPORT_ANALOG_BUTTON_THRESHOLD)
		{
			m_State.buttons |= 1 << (GamePadButtonLT+i);
		}
	}
}
