//
// usbgamepadps4.h
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
#ifndef _circle_usb_usbgamepadps4_h
#define _circle_usb_usbgamepadps4_h

#include <circle/usb/usbgamepad.h>
#include <circle/types.h>

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
        u8 dummy3[3];

        /* The rest is data for the touchpad */
        touchpadXY xy[3]; // It looks like it sends out three coordinates each time, this might be because the microcontroller inside the PS4 controller is much faster than the Bluetooth connection.
                          // The last data is read from the last position in the array while the oldest measurement is from the first position.
                          // The first position will also keep it's value after the finger is released, while the other two will set them to zero.
                          // Note that if you read fast enough from the device, then only the first one will contain any data.

        // The last three bytes are always: 0x00, 0x80, 0x00
} PACKED;

struct PS4Output {
        u8 rumbleState, bigRumble, smallRumble; // Rumble
        u8 red, green, blue; // RGB
        u8 flashOn, flashOff; // Time to flash bright/dark (255 = 2.5 seconds)
};

class CUSBGamePadPS4Device : public CUSBGamePadDevice
{
public:
	CUSBGamePadPS4Device (CUSBFunction *pFunction);
	~CUSBGamePadPS4Device (void);

	boolean Configure (void);

	const TGamePadState *GetReport (void);		// returns 0 on failure

private:
	void DecodeReport (const u8 *pReportBuffer);
	void SendLedRumbleCommand(void);

private:
	boolean m_bInterfaceOK;
	PS4Output ps4Output;
	u8 *outBuffer;
};

#endif
