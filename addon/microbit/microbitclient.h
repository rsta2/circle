//
// microbitclient.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2020  R. Stange <rsta2@o2online.de>
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
#ifndef _microbit_microbitclient_h
#define _microbit_microbitclient_h

#include <circle/usb/usbserial.h>
#include <circle/string.h>
#include <circle/types.h>

#define MICROBIT_OK		0
#define MICROBIT_ERROR		(-9999999)

#define MICROBIT_FALSE		0
#define MICROBIT_TRUE		1

class CMicrobitClient
{
public:
	CMicrobitClient (const char *pDeviceName = "utty1");
	~CMicrobitClient (void);

	boolean Initialize (void);

	// Microbit
	int GetTemperature (void);

	// Display
#define MICROBIT_POS_MIN	0
#define MICROBIT_POS_MAX	4
#define MICROBIT_BRIGHT_MIN	0
#define MICROBIT_BRIGHT_MAX	9
	int SetPixel (unsigned nPosX, unsigned nPosY, unsigned nBrightness);
	int ClearDisplay (void);
	int Show (const char *pString);
	int Scroll (const char *pString);
	int SetDisplay (boolean bOn);
#define MICROBIT_LIGHT_MIN	0
#define MICROBIT_LIGHT_MAX	255
	int GetLightLevel (void);

	// Buttons
#define MICROBIT_BUTTON_A	'A'
#define MICROBIT_BUTTON_B	'B'
	int IsPressed (char chButton);
	int WasPressed (char chButton);
	int GetPresses (char chButton);

	// Pins
#define MICROBIT_PIN_MIN	0
#define MICROBIT_PIN_MAX	20
#define MICROBIT_LOW		0
#define MICROBIT_HIGH		1
#define MICROBIT_ANALOG_MIN	0
#define MICROBIT_ANALOG_MAX	1023
#define MICROBIT_PERIOD_MIN_MS	1
#define MICROBIT_PERIOD_MIN_US	256
        int ReadDigital (unsigned nPin);
        int WriteDigital (unsigned nPin, unsigned nValue);
        int ReadAnalog (unsigned nPin);
        int WriteAnalog (unsigned nPin, unsigned nValue);
        int SetAnalogPeriod (unsigned nPin, unsigned nMillis);
        int SetAnalogPeriodMicroseconds (unsigned nPin, unsigned nMicros);
	int IsTouched (unsigned nPin);

	// Accelerometer
#define MICROBIT_ACCEL_MIN	(-2000)		// milli-g
#define MICROBIT_ACCEL_MAX	2000
        int GetAccelX (void);
        int GetAccelY (void);
        int GetAccelZ (void);
	int GetCurrentGesture (CString *pResult);
	int IsGesture (const char *pGesture);
	int WasGesture (const char *pGesture);

	// Compass
	int CalibrateCompass (void);
        int IsCompassCalibrated (void);
	int ClearCompassCalibration (void);
        int GetCompassX (void);			// returns -/+ nano tesla
        int GetCompassY (void);			// returns -/+ nano tesla
        int GetCompassZ (void);			// returns -/+ nano tesla
	int GetHeading (void);			// return 0..360 (0 is north)
        int GetFieldStrength (void);		// returns nano tesla

private:
	boolean SendCommand (const char *pCommand);
	boolean ReceiveResult (CString *pResult);

	int ReceiveInteger (void);

	int CheckStatus (void);

	static int ConvertInteger (const char *pString);

private:
	CString m_DeviceName;

	CUSBSerialDevice *m_pMicrobit;
};

#endif
