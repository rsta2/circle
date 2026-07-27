//
// usbgamepad8bitdo.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2026  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/usbgamepad8bitdo.h>
#include <circle/logger.h>
#include <circle/macros.h>
#include <circle/debug.h>
#include <assert.h>
#include <string.h>

static const char FromUSBPad8BitDo[] = "usbpad8bitdo";

struct T8BitDoReport
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

#define REPORT_DATA_SIZE	sizeof (T8BitDoReport)
// 8BitDo input reports contain 14 bytes of controller state followed by six
// reserved bytes. The interrupt endpoint has a 32-byte maximum packet size,
// so the host buffer must be large enough for the complete USB packet.
#define REPORT_SIZE_8BITDO	32 

CUSBGamePad8bitdoDevice::CUSBGamePad8bitdoDevice (CUSBFunction *pFunction)
: CUSBGamePadDevice (pFunction)
{
}

CUSBGamePad8bitdoDevice::~CUSBGamePad8bitdoDevice (void)
{
}

boolean CUSBGamePad8bitdoDevice::Configure (void)
{
	m_usReportSize = REPORT_SIZE_8BITDO;

	if (!CUSBGamePadDevice::Configure ())
	{
		CLogger::Get ()->Write (FromUSBPad8BitDo, LogError, "Cannot configure gamepad device");

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

	return TRUE;
}

void CUSBGamePad8bitdoDevice::ReportHandler (const u8 *pReport, unsigned nReportSize)
{
	if (   pReport != 0
	    && nReportSize >= REPORT_DATA_SIZE
	    && pReport[0] == (REPORT_HEADER & 0xFF)
	    && pReport[1] == (REPORT_HEADER >> 8)
		&& m_pStatusHandler != 0)
	{
		//debug_hexdump (pReport, m_usReportSize, FromUSBPad8BitDo);

		DecodeReport (pReport);

		(*m_pStatusHandler) (m_nDeviceNumber-1, &m_State);
	}
}

void CUSBGamePad8bitdoDevice::DecodeReport (const u8 *pReportBuffer)
{
	const T8BitDoReport *pReport = reinterpret_cast<const T8BitDoReport *> (pReportBuffer);
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

boolean CUSBGamePad8bitdoDevice::SetRumbleMode (TGamePadRumbleMode Mode)
{
	DMA_BUFFER (u8, Command, 8);
	memset (Command, 0, sizeof Command);
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

	return SendToEndpointOut (Command, sizeof Command);
}

