//
// usbgamepadps4.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
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

// Structs taken from project USB_Host_Shield_2.0
// https://github.com/felis/USB_Host_Shield_2.0
// Copyright (C) 2014 Kristian Lauszus, TKJ Electronics.
// Licensed under GPLv2
/*
 Contact information
 -------------------

 Kristian Lauszus, TKJ Electronics
 Web      :  http://www.tkjelectronics.com
 e-mail   :  kristianl@tkjelectronics.com
*/
union PS4Buttons {
    struct {
        u8 dpad : 4;
        u8 square : 1;
        u8 cross : 1;
        u8 circle : 1;
        u8 triangle : 1;

        u8 l1 : 1;
        u8 r1 : 1;
        u8 l2 : 1;
        u8 r2 : 1;
        u8 share : 1;
        u8 options : 1;
        u8 l3 : 1;
        u8 r3 : 1;

        u8 ps : 1;
        u8 touchpad : 1;
        u8 reportCounter : 6;
    } PACKED;
    u32 val : 24;
} PACKED;

struct touchpadXY {
        u8 dummy; // I can not figure out what this data is for, it seems to change randomly, maybe a timestamp?
        struct {
                u8 counter : 7; // Increments every time a finger is touching the touchpad
                u8 touching : 1; // The top bit is cleared if the finger is touching the touchpad
                u16 x : 12;
                u16 y : 12;
        } PACKED finger[2]; // 0 = first finger, 1 = second finger
} PACKED;

struct PS4Status {
        u8 battery : 4;
        u8 usb : 1;
        u8 audio : 1;
        u8 mic : 1;
        u8 unknown : 1; // Extension port?
} PACKED;

struct PS4Report {
	u8 reportID;
        /* Button and joystick values */
        u8 hatValue[4];
        PS4Buttons btn;
        u8 trigger[2];

        /* Gyro and accelerometer values */
        u8 dummy[3]; // First two looks random, while the third one might be some kind of status - it increments once in a while
        s16 gyroY, gyroZ, gyroX;
        s16 accX, accZ, accY;

        u8 dummy2[5];
        PS4Status status;
        u8 dummy3[2];

        /* The rest is data for the touchpad */
        u8 xyNum; // Number of valid entries in xy[]
        touchpadXY xy[3]; // It looks like it sends out three coordinates each time, this might be because the microcontroller inside the PS4 controller is much faster than the Bluetooth connection.
                        // The last data is read from the last position in the array while the oldest measurement is from the first position.
                        // The first position will also keep it's value after the finger is released, while the other two will set them to zero.
                        // Note that if you read fast enough from the device, then only the first one will contain any data.

        // The last three bytes are always: 0x00, 0x80, 0x00
} PACKED;

boolean CUSBGamePadPS4Device::s_bTouchpadEnabled = TRUE;

CUSBGamePadPS4Device::CUSBGamePadPS4Device (CUSBFunction *pFunction)
:	CUSBGamePadDevice (pFunction),
	m_bInterfaceOK (SelectInterfaceByClass (3, 0, 0)),
	outBuffer(nullptr),
	m_pMouseDevice (nullptr)
{
        m_Touchpad.bButtonPressed = FALSE;
	m_Touchpad.bTouched = FALSE;
}

CUSBGamePadPS4Device::~CUSBGamePadPS4Device (void)
{
	delete m_pMouseDevice;
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

	if (s_bTouchpadEnabled)
	{
		m_pMouseDevice = new CMouseDevice(1);
		assert (m_pMouseDevice != 0);
	}

	m_State.nbuttons = GAMEPAD_BUTTONS_WITH_TOUCHPAD;
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

	ps4Output.rumbleState = 0xF0; // 0xf0 disables the rumble motors, 0xf3 enables them
	ps4Output.smallRumble = 0x00; // Rumble (Right/weak)
	ps4Output.bigRumble = 0x00;   // Rumble (Left/strong)
	ps4Output.red = 0xFF; 		  // RGB Color (Red)
	ps4Output.green = 0xFF;	   	  // RGB Color (Green)
	ps4Output.blue = 0xFF;		  // RGB Color (Blue)
	ps4Output.flashOn = 0x7F;	  // Time to flash bright (255 = 2.5 seconds)
	ps4Output.flashOff = 0xFF;	  // Time to flash dark (255 = 2.5 seconds)
	SendLedRumbleCommand();
	ps4Output.red = 0x00;
	ps4Output.green = 0x00;
	ps4Output.blue = 0x00;
	CTimer::SimpleMsDelay(250);
	SendLedRumbleCommand();

	return StartRequest ();
}

void CUSBGamePadPS4Device::ReportHandler (const u8 *pReport, unsigned nReportSize)
{
	if (   pReport != 0
	    && nReportSize == REPORT_SIZE)
	{
		//debug_hexdump (pReport, m_usReportSize, FromUSBPadPS4);

		DecodeReport (pReport);

		if (m_pStatusHandler != 0)
		{
			(*m_pStatusHandler) (m_nDeviceNumber-1, &m_State);
		}

		if (m_pMouseDevice != 0)
		{
			HandleTouchpad (pReport);
		}
	}
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

	m_State.acceleration[0] = pReport->accX;
	m_State.acceleration[1] = pReport->accY;
	m_State.acceleration[2] = pReport->accZ;
	m_State.gyroscope[0] = pReport->gyroX;
	m_State.gyroscope[1] = pReport->gyroY;
	m_State.gyroscope[2] = pReport->gyroZ;
}

void CUSBGamePadPS4Device::HandleTouchpad (const u8 *pReportBuffer)
{
	const PS4Report *pReport = reinterpret_cast<const PS4Report *> (pReportBuffer);

	boolean bButtonChanged = FALSE;
	if (   ( pReport->btn.touchpad && !m_Touchpad.bButtonPressed)
	    || (!pReport->btn.touchpad &&  m_Touchpad.bButtonPressed))
	{
		m_Touchpad.bButtonPressed = pReport->btn.touchpad ? TRUE : FALSE;
		bButtonChanged = TRUE;
	}

	// get number of valid touchpad entries and validate it
	unsigned nEvents = pReport->xyNum;
	if (nEvents == 0 || nEvents > 3)
	{
		nEvents = 1;
	}

	// run through touchpad events, second finger is ignored
	int nDisplacementX = 0;
	int nDisplacementY = 0;
	for (unsigned i = 0; i < nEvents; i++)
	{
		if (pReport->xy[i].finger[0].touching == 0)
		{
			// if touchpad is touched and was touched before
			if (m_Touchpad.bTouched)
			{
				// valid move found, calculate displacement
				nDisplacementX = (int) pReport->xy[i].finger[0].x - m_Touchpad.usPosX;
				if (nDisplacementX < MOUSE_DISPLACEMENT_MIN)
				{
					nDisplacementX = MOUSE_DISPLACEMENT_MIN;
				}
				else if (nDisplacementX > MOUSE_DISPLACEMENT_MAX)
				{
					nDisplacementX = MOUSE_DISPLACEMENT_MAX;
				}

				nDisplacementY = (int) pReport->xy[i].finger[0].y - m_Touchpad.usPosY;
				if (nDisplacementY < MOUSE_DISPLACEMENT_MIN)
				{
					nDisplacementY = MOUSE_DISPLACEMENT_MIN;
				}
				else if (nDisplacementY > MOUSE_DISPLACEMENT_MAX)
				{
					nDisplacementY = MOUSE_DISPLACEMENT_MAX;
				}
			}

			// save this touchpad state as the previous one
			m_Touchpad.bTouched = TRUE;
			m_Touchpad.usPosX = pReport->xy[i].finger[0].x;
			m_Touchpad.usPosY = pReport->xy[i].finger[0].y;
		}
		else
		{
			m_Touchpad.bTouched = FALSE;
		}

		// if finger has moved or button state changed, report it
		if (   bButtonChanged
		    || nDisplacementX != 0
		    || nDisplacementY != 0)
		{
			assert (m_pMouseDevice != 0);
			m_pMouseDevice->ReportHandler (  m_Touchpad.bButtonPressed
						       ? MOUSE_BUTTON_LEFT : 0,
						       nDisplacementX, nDisplacementY, 0);

			bButtonChanged = FALSE;
			nDisplacementX = 0;
			nDisplacementY = 0;
		}
	}
}

boolean CUSBGamePadPS4Device::SetLEDMode (TGamePadLEDMode Mode)
{
	switch (Mode) {
    	        case GamePadLEDModeOn1:
        	       // Blue
        	       ps4Output.red = 0x00;
        	       ps4Output.green = 0x00;
        	       ps4Output.blue = 0xFF;
        	       ps4Output.flashOn = 0x7F;
        	       ps4Output.flashOff = 0xFF;
        	       break;
                case GamePadLEDModeOn2:
        	       // Red
                       ps4Output.red = 0xFF;
                       ps4Output.green = 0x00;
                       ps4Output.blue = 0x00;
                       ps4Output.flashOn = 0x7F;
                       ps4Output.flashOff = 0xFF;
                       break;
                case GamePadLEDModeOn3:
        	       // Magenta
                       ps4Output.red = 0xFF;
                       ps4Output.green = 0x00;
                       ps4Output.blue = 0xFF;
                       ps4Output.flashOn = 0x7F;
                       ps4Output.flashOff = 0xFF;
                       break;
                case GamePadLEDModeOn4:
        	       // Green
                       ps4Output.red = 0x00;
                       ps4Output.green = 0xFF;
                       ps4Output.blue = 0x00;
                       ps4Output.flashOn = 0x7F;
                       ps4Output.flashOff = 0xFF;
                       break;
                case GamePadLEDModeOn5:
        	       // Cyan
                       ps4Output.red = 0x00;
                       ps4Output.green = 0xFF;
                       ps4Output.blue = 0xFF;
                       ps4Output.flashOn = 0x7F;
                       ps4Output.flashOff = 0xFF;
                       break;
                case GamePadLEDModeOn6:
        	       // Yellow
                       ps4Output.red = 0xFF;
                       ps4Output.green = 0xFF;
                       ps4Output.blue = 0x00;
                       ps4Output.flashOn = 0x7F;
                       ps4Output.flashOff = 0xFF;
                       break;
                case GamePadLEDModeOn7:
        	       // White
                       ps4Output.red = 0xFF;
                       ps4Output.green = 0xFF;
                       ps4Output.blue = 0xFF;
                       ps4Output.flashOn = 0x7F;
                       ps4Output.flashOff = 0xFF;
                       break;
                default:
        	       ps4Output.red = 0x00;
                       ps4Output.green = 0x00;
                       ps4Output.blue = 0x00;
                       ps4Output.flashOn = 0x00;
                       ps4Output.flashOff = 0x00;
                       break;
       }
       return SendLedRumbleCommand();
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

void CUSBGamePadPS4Device::DisableTouchpad (void)
{
	s_bTouchpadEnabled = FALSE;
}
