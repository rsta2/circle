//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017  R. Stange <rsta2@o2online.de>
//
// This demo by John Bradley, https://github.com/flypie
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
#include <circle/gpiopin.h>
#include <circle/timer.h>

CKernel::CKernel (void)
:	m_Screen (0, 0)
{
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
	return m_Screen.Initialize ();
}

TShutdownMode CKernel::Run (void)
{
	CGPIOPin *AllPins[GPIO_PINS];
	bool      OldPins[GPIO_PINS] = {0};
	unsigned  i;

	for (i = 2; i < GPIO_PINS/4; i++)
	{
		AllPins[i] = new CGPIOPin (i, GPIOModeOutput);
	}

	for (i = GPIO_PINS/4; i < GPIO_PINS/2; i++)
	{
		AllPins[i] = new CGPIOPin (i, GPIOModeInput);
	}

	for (i = 1; 1; i++)
	{
		m_ActLED.On ();
		CTimer::SimpleMsDelay (200);

		m_ActLED.Off ();
		CTimer::SimpleMsDelay (500);

		for (unsigned j = 2; j < GPIO_PINS/4; j++)
		{
			if (i & 1)
			{
				AllPins[j]->Write (LOW);
			}
			else
			{
				AllPins[j]->Write (HIGH);
			}
		}

		for (unsigned j = GPIO_PINS/4; j < GPIO_PINS/2; j++)
		{
			bool In = AllPins[j]->Read () == HIGH;
			if (In != OldPins[j])
			{
				CString Message;
				Message.Format ("\rPin %u Changed state\n", j);
				m_Screen.Write ((const char *) Message, Message.GetLength ());

				OldPins[j] = In;
			}
		}

		CString Message;
		Message.Format ("\r%4u", i);
		m_Screen.Write ((const char *) Message, Message.GetLength ());
	}

	return ShutdownHalt;
}
