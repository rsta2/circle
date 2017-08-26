//
// hcsr04.h
//
// Driver for HC-SR04 Ultrasonic distance measuring module
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017  R. Stange <rsta2@o2online.de>
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
#ifndef _sensor_hcsr04_h
#define _sensor_hcsr04_h

#include <circle/gpiopin.h>
#include <circle/types.h>

class CHCSR04
{
public:
	CHCSR04 (unsigned nTriggerPin, unsigned nEchoPin);
	~CHCSR04 (void);

	boolean Initialize (void);

	boolean DoMeasurement (unsigned nTimeoutMicros = 30000);

	unsigned GetDistance (void) const;		// returns Millimeters

private:
	CGPIOPin m_TriggerPin;
	CGPIOPin m_EchoPin;

	unsigned m_nDistance;				// Millimeters
};

#endif
