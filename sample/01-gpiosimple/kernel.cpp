//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2023  R. Stange <rsta2@o2online.de>
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
#include "kernel.h"
#if RASPPI <= 4
	#include <circle/gpiopin.h>
#endif
#include <circle/timer.h>

CKernel::CKernel (void)
{
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
	return TRUE;
}

TShutdownMode CKernel::Run (void)
{
#if RASPPI <= 4
	CGPIOPin AudioLeft (GPIOPinAudioLeft, GPIOModeOutput);
	CGPIOPin AudioRight (GPIOPinAudioRight, GPIOModeOutput);
#endif

	// flash the Act LED 10 times and click on audio (3.5mm headphone jack)
	for (unsigned i = 1; i <= 10; i++)
	{
		m_ActLED.On ();
#if RASPPI <= 4
		AudioLeft.Invert ();
		AudioRight.Invert ();
#endif
		CTimer::SimpleMsDelay (200);

		m_ActLED.Off ();
		CTimer::SimpleMsDelay (500);
	}

	return ShutdownReboot;
}
