//
// usbgamepadxboxone.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2018-2021  R. Stange <rsta2@o2online.de>
//
// This driver was developed by:
//	Jose Luis Sanchez, jspeccy@gmail.com, jsanchezv@github.com
//
// Information to implement this was taken from:
//	https://github.com/quantus/xbox-one-controller-protocol
//	Copyright (C) 2016 Pekka Pöyry
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
#include <circle/usb/usbgamepadxboxone.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/synchronize.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <circle/macros.h>
#include <circle/debug.h>
#include <assert.h>

struct TXboxOneReport {
	u8 type;
	u8 const_0;
	u16 id;

	bool sync : 1;    // Not used in this driver
	bool dummy1 : 1;  // Always 0.
	bool start : 1;
	bool back : 1;

	bool a : 1;
	bool b : 1;
	bool x : 1;
	bool y : 1;

	bool dpad_up : 1;
	bool dpad_down : 1;
	bool dpad_left : 1;
	bool dpad_right : 1;

	bool bumper_left : 1;
	bool bumper_right : 1;
	bool stick_left_click : 1;
	bool stick_right_click : 1;

	u16 trigger_left;
	u16 trigger_right;

	s16 stick_left_x;
	s16 stick_left_y;
	s16 stick_right_x;
	s16 stick_right_y;
} PACKED;

#define REPORT_BUTTONS			14

#define REPORT_ANALOG_BUTTONS		2
#define REPORT_ANALOG_BUTTON_MINIMUM	0
#define REPORT_ANALOG_BUTTON_THRESHOLD	128
#define REPORT_ANALOG_BUTTON_MAXIMUM	255

#define REPORT_AXES			4
#define REPORT_AXES_MINIMUM		(-32768)
#define REPORT_AXES_MAXIMUM		32767

#define REPORT_SIZE			64

static const char FromUSBPadXboxOne[] = "usbpadxboxone";

CUSBGamePadXboxOneDevice::CUSBGamePadXboxOneDevice (CUSBFunction *pFunction)
:	CUSBGamePadDevice (pFunction), seqNum(0)
{
}

CUSBGamePadXboxOneDevice::~CUSBGamePadXboxOneDevice (void)
{
}

boolean CUSBGamePadXboxOneDevice::Configure (void)
{
	m_usReportSize = REPORT_SIZE;

	if (!CUSBGamePadDevice::Configure ())
	{
		CLogger::Get ()->Write (FromUSBPadXboxOne, LogError, "Cannot configure gamepad device");

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

	// Send Start controller (with input) command
	DMA_BUFFER (u8, command_start, 5) = { 0x05, 0x20, 0x00, 0x01, 0x00 };
	if (!SendToEndpointOut (command_start, 5)) {
		CLogger::Get ()->Write (FromUSBPadXboxOne, LogError, "command_start failed!");
		// That's an unrecoverable error
		return FALSE;
	}

	return StartRequest ();
}

void CUSBGamePadXboxOneDevice::ReportHandler (const u8 *pReport, unsigned reportSize)
{
	//debug_hexdump (pReport, REPORT_SIZE, FromUSBPadXboxOne);
	//CLogger::Get ()->Write (FromUSBPadXboxOne, LogNotice, "Received report of %u bytes", reportSize);
	if (pReport == nullptr)
		return;

	// Controller sends this packet when XBox button is pressed, and needs ACK
	// or resends the packet forever
	if (reportSize == 6 && pReport[0] == 0x07 && pReport[1] == 0x30) {
		DMA_BUFFER (u8, mode_report_ack, 13) = {
			0x01, 0x20, 0x00, 0x09, 0x00, 0x07, 0x20, 0x02,
			0x00, 0x00, 0x00, 0x00, 0x00
		};
		mode_report_ack[2] = pReport[2];
		if (!SendToEndpointOutAsync (mode_report_ack, 13))
		{
			CLogger::Get ()->Write (FromUSBPadXboxOne, LogError, "ACK send failed!");
		}
		return;
	}

	// Controller sends "heartbeat" packets periodically starting with
	// 0x03, 0x20. We aren't interested in them.
	if (m_pStatusHandler != nullptr && reportSize == 18 && pReport[0] == 0x20)
	{
		DecodeReport (pReport);

		(*m_pStatusHandler) (m_nDeviceNumber-1, &m_State);
	}
}

void CUSBGamePadXboxOneDevice::DecodeReport (const u8 *pReportBuffer)
{
	const TXboxOneReport *pReport = reinterpret_cast<const TXboxOneReport *> (pReportBuffer);
	//debug_hexdump (pReport, REPORT_SIZE, FromUSBPadXboxOne);

	m_State.buttons = 0;

	if (pReport->start)
		m_State.buttons |= GamePadButtonStart;
	if (pReport->back)
		m_State.buttons |= GamePadButtonBack;

	if (pReport->a)
		m_State.buttons |= GamePadButtonA;
	if (pReport->b)
		m_State.buttons |= GamePadButtonB;
	if (pReport->x)
		m_State.buttons |= GamePadButtonX;
	if (pReport->y)
		m_State.buttons |= GamePadButtonY;

	if (pReport->dpad_up)
		m_State.buttons |= GamePadButtonUp;
	if (pReport->dpad_down)
		m_State.buttons |= GamePadButtonDown;
	if (pReport->dpad_left)
		m_State.buttons |= GamePadButtonLeft;
	if (pReport->dpad_right)
		m_State.buttons |= GamePadButtonRight;

	if (pReport->bumper_left)
		m_State.buttons |= GamePadButtonLB;
	if (pReport->bumper_right)
		m_State.buttons |= GamePadButtonRB;
	if (pReport->stick_left_click)
		m_State.buttons |= GamePadButtonL3;
	if (pReport->stick_right_click)
		m_State.buttons |= GamePadButtonR3;

	m_State.axes[GamePadAxisButtonLT].value = pReport->trigger_left >> 2;
	if (m_State.axes[GamePadAxisButtonLT].value >= REPORT_ANALOG_BUTTON_THRESHOLD)
		m_State.buttons |= GamePadButtonLT;

	m_State.axes[GamePadAxisButtonRT].value = pReport->trigger_right >> 2;
	if (m_State.axes[GamePadAxisButtonRT].value >= REPORT_ANALOG_BUTTON_THRESHOLD)
		m_State.buttons |= GamePadButtonRT;

	m_State.axes[GamePadAxisLeftX].value = (unsigned)(pReport->stick_left_x - REPORT_AXES_MINIMUM) >> 8;
	u8 rAxis = (unsigned)(pReport->stick_left_y - REPORT_AXES_MINIMUM) >> 8;
	m_State.axes[GamePadAxisLeftY].value = GAMEPAD_AXIS_DEFAULT_MAXIMUM - rAxis; // Inverted axis

	m_State.axes[GamePadAxisRightX].value = (unsigned)(pReport->stick_right_x - REPORT_AXES_MINIMUM) >> 8;
	rAxis = (unsigned)(pReport->stick_right_y - REPORT_AXES_MINIMUM) >> 8;
	m_State.axes[GamePadAxisRightY].value = GAMEPAD_AXIS_DEFAULT_MAXIMUM - rAxis; // Inverted axis
}

boolean CUSBGamePadXboxOneDevice::SetRumbleMode (TGamePadRumbleMode Mode)
{
	// Command taken from Linux driver xpad.c
	DMA_BUFFER (u8, Command, 13) = { 0x09, 0x00, 0x00, 0x09, 0x00, 0x0f, 0x00, 0x00,
					 0x00, 0x00, 0xFF, 0x00, 0xFF };

	switch (Mode)
	{
		case GamePadRumbleModeOff:
			break;

		case GamePadRumbleModeLow:
			Command[9] = 0xFF;
			break;

		case GamePadRumbleModeHigh:
			Command[8] = 0xFF;
			break;

		default:
			assert (0);
			return FALSE;
	}

	Command[2] = seqNum++;
	return SendToEndpointOut (Command, 13);
}
