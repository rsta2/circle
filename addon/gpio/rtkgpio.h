//
// rtkgpio.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2020  H. Kocevar <hinxx@protonmail.com>
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
#ifndef _gpio_rtkgpio_h
#define _gpio_rtkgpio_h

#include <circle/usb/usbserialch341.h>
#include <circle/types.h>

enum TRtkGpioDirection
{
	RtkGpioDirectionOutput,
	RtkGpioDirectionInput
};

enum TRtkGpioLevel
{
	RtkGpioLevelUnknown,
	RtkGpioLevelLow,
	RtkGpioLevelHigh
};

enum TRtkGpioPull
{
	RtkGpioPullNone,
	RtkGpioPullDown,
	RtkGpioPullUp
};

class CRTKGpioDevice
{
public:
	CRTKGpioDevice (CUSBSerialCH341Device *pCH341);
	~CRTKGpioDevice ();

	boolean Initialize (void);
	boolean GetVersion (void);

	boolean SetPinDirection (unsigned nPin, TRtkGpioDirection eDirection);
	boolean SetPinDirectionOutput (unsigned nPin);
	boolean SetPinDirectionInput (unsigned nPin);

	boolean SetPinPull (unsigned nPin, TRtkGpioPull ePull);
	boolean SetPinPullUp (unsigned nPin);
	boolean SetPinPullDown (unsigned nPin);
	boolean SetPinPullNone (unsigned nPin);

	boolean SetPinLevel (unsigned nPin, TRtkGpioLevel eLevel);
	boolean SetPinLevelLow (unsigned nPin);
	boolean SetPinLevelHigh (unsigned nPin);

	TRtkGpioLevel GetPinLevel (unsigned nPin);

private:
	int Read (void *pBuffer, size_t nCount);

private:
	CUSBSerialCH341Device	*m_pCH341;
};

#endif
