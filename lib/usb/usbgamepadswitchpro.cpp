//
// usbgamepadswitchpro.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
//
// This driver was developed by:
//	Jose Luis Sanchez, jspeccy@gmail.com, jsanchezv@github.com
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
#include <circle/usb/usbgamepadswitchpro.h>
#include <circle/usb/usbhid.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/synchronize.h>
#include <circle/logger.h>
#include <circle/macros.h>
#include <circle/debug.h>
#include <circle/util.h>
#include <assert.h>


#define REPORT_SIZE			64
#define OUTPUT_REPORT_SIZE		32
#define REPORT_AXIS 			4
#define REPORT_ANALOG_BUTTONS           0
#define REPORT_AXES_MINIMUM		0
#define REPORT_AXES_MAXIMUM		255
#define REPORT_BUTTONS			18
#define REPORT_HATS			0

static const char FromUSBPadSwitchPro[] = "usbpadswitchpro";

union ProConButtons {
    struct {
        u8 y : 1;
        u8 x : 1;
        u8 b : 1;
        u8 a : 1;
        u8 void1 : 2;
	u8 r : 1;
	u8 rz : 1;

        u8 plus : 1;
        u8 less : 1;
        u8 sr : 1;
        u8 sl : 1;
        u8 home : 1;
        u8 capture : 1;
        u8 void2 : 2;

        u8 down : 1;
        u8 up : 1;
	u8 right : 1;
	u8 left : 1;
	u8 void3 : 2;
	u8 l : 1;
	u8 lz : 1;
    } PACKED;
    u8 val[3];
} PACKED;

struct ProConReport {
	u8 reportID;	// 0x30, but others ID exist
	u8 seqNum;		// sequential number
	u8 dummy;		// unknown
	ProConButtons btn;
	u16 axisLx; // axis mask is 0x0FF0
	u8 axisLy;  // inverted axis
	u16 axisRx; // axis mask is 0x0FF0
	u8 axisRy;  // inverted axis
	u8 dummy2;
	u8 zeroes[51];
} PACKED;

CUSBGamePadSwitchProDevice::CUSBGamePadSwitchProDevice (CUSBFunction *pFunction)
:	CUSBGamePadDevice (pFunction),
	m_bInterfaceOK (SelectInterfaceByClass (3, 0, 0)),
        rumbleCounter(0)
{
}

CUSBGamePadSwitchProDevice::~CUSBGamePadSwitchProDevice (void)
{
}

boolean CUSBGamePadSwitchProDevice::Configure (void)
{
	if (!m_bInterfaceOK)
	{
		ConfigurationError (FromUSBPadSwitchPro);

		return FALSE;
	}

	m_usReportSize = REPORT_SIZE;

	if (!CUSBGamePadDevice::Configure ())
	{
		CLogger::Get ()->Write (FromUSBPadSwitchPro, LogError, "Cannot configure gamepad device");

		return FALSE;
	}

	m_State.nbuttons = GAMEPAD_BUTTONS_ALTERNATIVE;
	m_State.nhats = REPORT_HATS;
	m_State.naxes = REPORT_AXIS + REPORT_ANALOG_BUTTONS;
	for (int axis = 0; axis < REPORT_AXIS + REPORT_ANALOG_BUTTONS; axis++) {
		m_State.axes[axis].minimum = REPORT_AXES_MINIMUM;
		m_State.axes[axis].maximum = REPORT_AXES_MAXIMUM;
	}

        DMA_BUFFER (u8, ReportBuffer, m_usReportSize);

	// When Pro Controller starts to work, a first report is enqueued carrying
        // 6 bytes with the Bluetooth MAC, starting at byte 4, in reverse order
	if (ReceiveFromEndpointIn(ReportBuffer, m_usReportSize) > 0)
	{
                //CLogger::Get ()->Write (FromUSBPadSwitchPro, LogError, "Unqueued pending data");
		//debug_hexdump (ReportBuffer, m_usReportSize, FromUSBPadSwitchPro);
	}

	// This command switches "something" in the controller
        DMA_BUFFER (u8, switch_baudrate, 2) = { 0x80, 0x03 };
        if (!SendToEndpointOut(switch_baudrate, 2)) {
                CLogger::Get ()->Write (FromUSBPadSwitchPro, LogError, "switch_baudrate command failed!");
                return FALSE;
        }

	// Is needed to read controller answer, a report with 0x81, 0x03 and zeroes
	if (ReceiveFromEndpointIn(ReportBuffer, m_usReportSize) <= 0)
	{
                CLogger::Get ()->Write (FromUSBPadSwitchPro, LogError, "switch_baudrate answer failed!");
                //debug_hexdump (ReportBuffer, m_usReportSize, FromUSBPadSwitchPro);
                return FALSE;
        }

	// Check ACK
	if (ReportBuffer[0] != 0x81 || ReportBuffer[1] != 0x03) {
		CLogger::Get ()->Write (FromUSBPadSwitchPro, LogError, "switch_baudrate command failed!");
		//debug_hexdump (ReportBuffer, m_usReportSize, FromUSBPadSwitchPro);
                return FALSE;
	}

	// This command switches "another something" in the controller
        DMA_BUFFER (u8, handshake, 2) = { 0x80, 0x02 };
        if (!SendToEndpointOut(handshake, 2)) {
                CLogger::Get ()->Write (FromUSBPadSwitchPro, LogError, "handshake command failed!");
                return FALSE;
        }

        // Receive ACK
	if (ReceiveFromEndpointIn(ReportBuffer, m_usReportSize) <= 0)
	{
                CLogger::Get ()->Write (FromUSBPadSwitchPro, LogError, "handshake answer failed!");
                //debug_hexdump (ReportBuffer, m_usReportSize, FromUSBPadSwitchPro);
                return FALSE;
	}

	// Check ACK
	if (ReportBuffer[0] != 0x81 || ReportBuffer[1] != 0x02) {
		CLogger::Get ()->Write (FromUSBPadSwitchPro, LogError, "handshake command failed!");
		//debug_hexdump (ReportBuffer, m_usReportSize, FromUSBPadSwitchPro);
                return FALSE;
	}

	// This command switches the controller to a HID mode, from here all works
	// like a "standard" controller (ehem!). No ACK to this command.
        DMA_BUFFER (u8, hid_only_mode, 2) = { 0x80, 0x04 };		// DMA buffer
        if (!SendToEndpointOut(hid_only_mode, 2)) {
                CLogger::Get ()->Write (FromUSBPadSwitchPro, LogError, "hid_only_mode command failed!");
                return FALSE;
        }

        SetLEDMode(static_cast<TGamePadLEDMode>(m_nDeviceNumber));
        // Get the answer to LED commmand, sometimes gets a standard input report (0x30)
        // instead a LED report (0x81). Anyway, consume the answer.
        if (ReceiveFromEndpointIn(ReportBuffer, m_usReportSize) <= 0)
	{
                CLogger::Get ()->Write (FromUSBPadSwitchPro, LogError, "SetLEDMode answer failed!");
                //debug_hexdump (ReportBuffer, m_usReportSize, FromUSBPadSwitchPro);
                // Isn't a critical error, so continue, awaiting to be lucky...
        }

	return StartRequest();
}

void CUSBGamePadSwitchProDevice::DecodeReport (const u8 *pReportBuffer)
{
	const ProConReport *pReport = reinterpret_cast<const ProConReport *> (pReportBuffer);

	// Other reports can be received with reportID == 0x81 after some commands,
	// (following a LED change, by example, can be more commands are affected)
	// but can be ignored even if they carry valid information (generally they
        // carry it, at a different offset).
	if (pReport->reportID != 0x30) {
		CLogger::Get ()->Write (FromUSBPadSwitchPro, LogNotice, "Unhandled reportID = %02X", pReport->reportID);
		//debug_hexdump (pReportBuffer, m_usReportSize, FromUSBPadSwitchPro);
		return;
	}

        m_State.buttons = 0;

        if (pReport->btn.y)
                m_State.buttons |= GamePadButtonY;
        if (pReport->btn.x)
                m_State.buttons |= GamePadButtonX;
        if (pReport->btn.b)
                m_State.buttons |= GamePadButtonB;
        if (pReport->btn.a)
                m_State.buttons |= GamePadButtonA;
        if (pReport->btn.r)
                m_State.buttons |= GamePadButtonR;
        if (pReport->btn.rz)
                m_State.buttons |= GamePadButtonRZ;

        if (pReport->btn.plus)
                m_State.buttons |= GamePadButtonPlus;
        if (pReport->btn.less)
                m_State.buttons |= GamePadButtonMinus;
        if (pReport->btn.sr)
                m_State.buttons |= GamePadButtonSR;
        if (pReport->btn.sl)
                m_State.buttons |= GamePadButtonSL;
        if (pReport->btn.home)
                m_State.buttons |= GamePadButtonHome;
        if (pReport->btn.capture)
                m_State.buttons |= GamePadButtonCapture;

        if (pReport->btn.down)
                m_State.buttons |= GamePadButtonDown;
        if (pReport->btn.up)
                m_State.buttons |= GamePadButtonUp;
        if (pReport->btn.right)
                m_State.buttons |= GamePadButtonRight;
        if (pReport->btn.left)
                m_State.buttons |= GamePadButtonLeft;
        if (pReport->btn.l)
                m_State.buttons |= GamePadButtonL;
        if (pReport->btn.lz)
                m_State.buttons |= GamePadButtonLZ;

        m_State.axes[GamePadAxisLeftX].value = (pReport->axisLx >> 4) & 0xff;
        m_State.axes[GamePadAxisLeftY].value = 255 - pReport->axisLy;
	m_State.axes[GamePadAxisRightX].value = (pReport->axisRx >> 4) & 0xff;
        m_State.axes[GamePadAxisRightY].value = 255 - pReport->axisRy;

        //debug_hexdump (pReportBuffer, 16, FromUSBPadSwitchPro);
}

boolean CUSBGamePadSwitchProDevice::SetLEDMode(TGamePadLEDMode Mode)
{
        DMA_BUFFER (u8, led_command_calibrated, 12) = { 0x01, 0x00, 0x00, 0x01,
							0x40, 0x40, 0x00, 0x01,
							0x40, 0x40, 0x30, 0xff };

	// This command changes the leds, with a bit per LED, low nibble from position 19
	// bit0 == LED1, bit1 == LED2, bit2 == LED3, bit3 = LED4
	led_command_calibrated[11] = static_cast<u8>(Mode);
	led_command_calibrated[1] = rumbleCounter++ & 0x0f;
	if (!SendToEndpointOut(led_command_calibrated, 12)) {
		CLogger::Get ()->Write (FromUSBPadSwitchPro, LogError, "led_command failed!");
                return FALSE;
	}
	return TRUE;
}

boolean CUSBGamePadSwitchProDevice::SetRumbleMode (TGamePadRumbleMode Mode)
{
        DMA_BUFFER (u8, rumble_command, 10) = { 0x10, 0x00, 0x80, 0x00, 0x00, 0x00,
						0x80, 0x00, 0x00, 0x00 };

        switch (Mode) {
    	        case GamePadRumbleModeOff:
                        // do nothing
        	        break;
                case GamePadRumbleModeLow:
                        rumble_command[6] = 0x98;
                        rumble_command[7] = 0x20;
                        rumble_command[8] = 0x62;
                        rumble_command[9] = 0xFF;
                        break;
                case GamePadRumbleModeHigh:
                        rumble_command[2] = 0x80;
                        rumble_command[3] = 0x20;
                        rumble_command[4] = 0x62;
                        rumble_command[5] = 0xFF;
                        break;
                default:
                        return TRUE;
        }
        rumble_command[1] = rumbleCounter++ & 0x0f;

        if (!SendToEndpointOut(rumble_command, 10)) {
		CLogger::Get ()->Write (FromUSBPadSwitchPro, LogError, "rumble_command failed!");
                return FALSE;
	}
	return TRUE;
}
