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
#define REPORT_AXIS 			MAX_AXIS
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
	m_State.naxes = REPORT_AXIS;
	for (int axis = 0; axis < REPORT_AXIS; axis++) {
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
	m_State.axes[0].value = pReportBuffer[1]; // Left Stick X (0 = left)
	m_State.axes[1].value = pReportBuffer[2]; // Left Stick Y (0 = up)
	m_State.axes[2].value = pReportBuffer[3]; // Right Stick X
	m_State.axes[3].value = pReportBuffer[4]; // Right Stick Y
	m_State.axes[4].value = pReportBuffer[8]; // Left Trigger [L2] (0 = released, 0xFF = fully pressed)
	m_State.axes[5].value = pReportBuffer[9]; // Right Trigger [R2]

	m_State.hats[0] = pReportBuffer[5] & 0x0F; // D-PAD (hat format, 0x08 is released,
		 									   // 0=N, 1=NE, 2=E, 3=SE, 4=S, 5=SW, 6=W, 7=NW)

	m_State.buttons = (pReportBuffer[5] & 0xF0) << 6; // TRIANGLE, CIRCLE, X, SQUARE
	m_State.buttons |= (pReportBuffer[6] << 2);		  // R3, L3, OPTIONS, SHARE, R2, L2, R1, L1
	m_State.buttons |= (pReportBuffer[7] & 0x03);	  // T-PAD, PS
}

void CUSBGamePadPS4Device::SendLedRumbleCommand(void) {
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
	}
	//CLogger::Get ()->Write (FromUSBPadPS4, LogNotice, "LEDs & Rumble configured");
}
