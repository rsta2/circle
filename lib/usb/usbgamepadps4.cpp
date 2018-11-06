//
// usbgamepadps4.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2018  R. Stange <rsta2@o2online.de>
//
// This driver was developed by:
//	Jose Luis Sanchez, http://jspeccy.speccy.org/
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
#include <circle/usb/usbgamepadps4.h>
#include <circle/usb/usbhid.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/logger.h>
#include <circle/macros.h>
#include <circle/debug.h>
#include <circle/util.h>
#include <assert.h>


#define REPORT_SIZE				64
#define OUTPUT_REPORT_SIZE		32
#define REPORT_AXIS 			4
#define REPORT_ANALOG_BUTTONS	2
#define REPORT_AXES_MINIMUM		0
#define REPORT_AXES_MAXIMUM		255
#define REPORT_BUTTONS			14
#define REPORT_HATS				1

static const char FromUSBPadPS4[] = "usbpadps4";

CUSBGamePadPS4Device::CUSBGamePadPS4Device (CUSBFunction *pFunction)
:	CUSBGamePadDevice (pFunction),
	m_bInterfaceOK (SelectInterfaceByClass (3, 0, 0))
{
}

CUSBGamePadPS4Device::~CUSBGamePadPS4Device (void)
{
	delete[] outBuffer;
}

boolean CUSBGamePadPS4Device::Configure (void)
{
	if (!m_bInterfaceOK)
	{
		ConfigurationError (FromUSBPadPS4);

		return FALSE;
	}

	m_usReportSize = REPORT_SIZE;

	if (!CUSBGamePadDevice::Configure ())
	{
		CLogger::Get ()->Write (FromUSBPadPS4, LogError, "Cannot configure gamepad device");

		return FALSE;
	}

	m_State.nbuttons = REPORT_BUTTONS;
	m_State.nhats = REPORT_HATS;
	m_State.naxes = REPORT_AXIS + REPORT_ANALOG_BUTTONS;
	for (int axis = 0; axis < REPORT_AXIS + REPORT_ANALOG_BUTTONS; axis++) {
		m_State.axes[axis].minimum = REPORT_AXES_MINIMUM;
		m_State.axes[axis].maximum = REPORT_AXES_MAXIMUM;
	}

	outBuffer = new u8[OUTPUT_REPORT_SIZE];
	memset(outBuffer, 0, OUTPUT_REPORT_SIZE);
	outBuffer[0]  = 0x05; // REPORT ID
	outBuffer[1]  = 0x07;
	outBuffer[2]  = 0x04;

	ps4Output.rumbleState = 0xF3; // 0xf0 disables the rumble motors, 0xf3 enables them
	ps4Output.smallRumble = 0xFF; // Rumble (Right/weak)
	ps4Output.bigRumble = 0x00;   // Rumble (Left/strong)
	ps4Output.red = 0xFF; 		  // RGB Color (Red)
	ps4Output.green = 0xFF;	   	  // RGB Color (Green)
	ps4Output.blue = 0xFF;		  // RGB Color (Blue)
	ps4Output.flashOn = 0x7F;	  // Time to flash bright (255 = 2.5 seconds)
	ps4Output.flashOff = 0xFE;	  // Time to flash dark (255 = 2.5 seconds)
	SendLedRumbleCommand();
	ps4Output.rumbleState = 0xF0;
	ps4Output.smallRumble = 0x00;
	ps4Output.red = 0x00;
	ps4Output.green = 0x00;
	ps4Output.blue = 0x00;
	CTimer::SimpleMsDelay(250);
	SendLedRumbleCommand();

	return TRUE;
}

const TGamePadState *CUSBGamePadPS4Device::GetReport (void)
{
	u8 ReportBuffer[m_usReportSize];
	if (GetHost ()->ControlMessage (GetEndpoint0 (),
					REQUEST_IN | REQUEST_CLASS | REQUEST_TO_INTERFACE,
					GET_REPORT, (REPORT_TYPE_INPUT << 8) | 0x11,
					GetInterfaceNumber (),
					ReportBuffer, m_usReportSize) <= 0)
	{
		return 0;
	}

	DecodeReport (ReportBuffer);

	return &m_State;
}

/*
 * http://eleccelerator.com/wiki/index.php?title=DualShock_4#Report_Structure
 */
void CUSBGamePadPS4Device::DecodeReport (const u8 *pReportBuffer) {
	const PS4Report *pReport = reinterpret_cast<const PS4Report *> (pReportBuffer);

	m_State.axes[GamePadAxisLeftX].value = pReport->hatValue[0]; // Left Stick X (0 = left)
	m_State.axes[GamePadAxisLeftY].value = pReport->hatValue[1]; // Left Stick Y (0 = up)
	m_State.axes[GamePadAxisRightX].value = pReport->hatValue[2]; // Right Stick X
	m_State.axes[GamePadAxisRightY].value = pReport->hatValue[3]; // Right Stick Y
	m_State.axes[GamePadAxisButtonL2].value = pReport->trigger[0]; // Left Trigger [L2] (0 = released, 0xFF = fully pressed)
	m_State.axes[GamePadAxisButtonR2].value = pReport->trigger[1]; // Right Trigger [R2]

	m_State.hats[0] = pReport->btn.dpad; // D-PAD (hat format, 0x08 is released,
		 								 // 0=N, 1=NE, 2=E, 3=SE, 4=S, 5=SW, 6=W, 7=NW

	static u32 hat2buttons[9] = { GamePadButtonUp, (GamePadButtonUp | GamePadButtonRight),
								GamePadButtonRight, (GamePadButtonRight | GamePadButtonDown),
								GamePadButtonDown, (GamePadButtonDown | GamePadButtonLeft),
								GamePadButtonLeft, (GamePadButtonLeft| GamePadButtonUp), 0x00 };
	m_State.buttons = hat2buttons[pReport->btn.dpad];

	if (pReport->btn.triangle)
		m_State.buttons |= GamePadButtonTriangle;
	if (pReport->btn.circle)
		m_State.buttons |= GamePadButtonCircle;
	if (pReport->btn.cross)
		m_State.buttons |= GamePadButtonCross;
	if (pReport->btn.square)
		m_State.buttons |= GamePadButtonSquare;

	if (pReport->btn.r3)
		m_State.buttons |= GamePadButtonR3;
	if (pReport->btn.l3)
		m_State.buttons |= GamePadButtonL3;
	if (pReport->btn.options)
		m_State.buttons |= GamePadButtonOptions;
	if (pReport->btn.share)
		m_State.buttons |= GamePadButtonShare;
	if (pReport->btn.r2)
		m_State.buttons |= GamePadButtonR2;
	if (pReport->btn.l2)
		m_State.buttons |= GamePadButtonL2;
	if (pReport->btn.r1)
		m_State.buttons |= GamePadButtonR1;
	if (pReport->btn.l1)
		m_State.buttons |= GamePadButtonL1;

	if (pReport->btn.touchpad)
		m_State.buttons |= GamePadButtonTouchpad;
	if (pReport->btn.ps)
		m_State.buttons |= GamePadButtonPS;
}

boolean CUSBGamePadPS4Device::SetLEDMode (u32 nRGB, u8 uchTimeOn, u8 uchTimeOff)
{
	ps4Output.red = (u8)(nRGB >> 16);
	ps4Output.green = (u8)(nRGB >> 8);
	ps4Output.blue = (u8)nRGB;
	ps4Output.flashOn = uchTimeOn;
	ps4Output.flashOff = uchTimeOff;
	return SendLedRumbleCommand();
}

boolean CUSBGamePadPS4Device::SetRumbleMode (TGamePadRumbleMode Mode)
{
	switch (Mode) {
		case GamePadRumbleModeOff:
			ps4Output.rumbleState = 0xf0;
			ps4Output.smallRumble = 0x00;
			ps4Output.bigRumble = 0x00;
			break;
		case GamePadRumbleModeLow:
			ps4Output.rumbleState = 0xf3;
			ps4Output.smallRumble = 0xff;
			ps4Output.bigRumble = 0;
			break;
		case GamePadRumbleModeHigh:
			ps4Output.rumbleState = 0xf3;
			ps4Output.smallRumble = 0;
			ps4Output.bigRumble = 0xff;
			break;
		default:
			return TRUE;
	}
	return SendLedRumbleCommand();
}

boolean CUSBGamePadPS4Device::SendLedRumbleCommand(void)
{
	outBuffer[3]  = ps4Output.rumbleState; // 0xf0 disables the rumble motors, 0xf3 enables them
	outBuffer[4]  = ps4Output.smallRumble; // Rumble (Right/weak)
	outBuffer[5]  = ps4Output.bigRumble;   // Rumble (Left/strong)
	outBuffer[6]  = ps4Output.red; 		   // RGB Color (Red)
	outBuffer[7]  = ps4Output.green;	   // RGB Color (Green)
	outBuffer[8]  = ps4Output.blue;		   // RGB Color (Blue)
	outBuffer[9]  = ps4Output.flashOn;	   // Time to flash bright (255 = 2.5 seconds)
	outBuffer[10] = ps4Output.flashOff;	   // Time to flash dark (255 = 2.5 seconds)
	if (!SendToEndpointOut(outBuffer, OUTPUT_REPORT_SIZE)) {
		CLogger::Get ()->Write (FromUSBPadPS4, LogError, "Cannot configure LEDs & Rumble");
		return FALSE;
	}
	return TRUE;
	//CLogger::Get ()->Write (FromUSBPadPS4, LogNotice, "LEDs & Rumble configured");
}
