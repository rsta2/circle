//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2016  R. Stange <rsta2@o2online.de>
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
#include <circle/string.h>

#define SPI_CHIP_SELECT		0		// 0 or 1

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_DIO (&m_SPIMaster, SPI_CHIP_SELECT)
{
	m_ActLED.Blink (5);	// show we are alive
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;

	if (bOK)
	{
		bOK = m_Screen.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Serial.Initialize (115200);
	}

	if (bOK)
	{
		CDevice *pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
		if (pTarget == 0)
		{
			pTarget = &m_Screen;
		}

		bOK = m_Logger.Initialize (pTarget);
	}

	if (bOK)
	{
		bOK = m_Interrupt.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Timer.Initialize ();
	}

	if (bOK)
	{
		bOK = m_SPIMaster.Initialize ();
	}

	if (bOK)
	{
		bOK = m_DIO.Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	m_Logger.Write (FromKernel, LogNotice, "spi_dio v%.1f found", (double) m_DIO.GetVersion () / 10.0);

#if 1
	// Little "light show"
	m_DIO.SetModes (  DIO_MASK (DIO_FUNC_IO0, DIO_OUTPUT)
			| DIO_MASK (DIO_FUNC_IO1, DIO_OUTPUT)
			| DIO_MASK (DIO_FUNC_IO2, DIO_OUTPUT)
			| DIO_MASK (DIO_FUNC_IO3, DIO_OUTPUT)
			| DIO_MASK (DIO_FUNC_IO4, DIO_OUTPUT)
			| DIO_MASK (DIO_FUNC_IO5, DIO_OUTPUT)
			| DIO_MASK (DIO_FUNC_IO6, DIO_OUTPUT));

	while (1)
	{
		for (unsigned nFunc = DIO_FUNC_IO0; nFunc <= DIO_FUNC_MAX; nFunc++)
		{
			m_DIO.SetOutput (nFunc, DIO_HIGH);

			m_Timer.MsDelay (100);
		}

		for (unsigned nFunc = DIO_FUNC_IO0; nFunc <= DIO_FUNC_MAX; nFunc++)
		{
			m_DIO.SetOutput (nFunc, DIO_LOW);

			m_Timer.MsDelay (100);
		}
	}
#elif 1
	// Displaying dio inputs
	m_DIO.SetModes (  DIO_MASK (DIO_FUNC_IO0, DIO_INPUT)
			| DIO_MASK (DIO_FUNC_IO1, DIO_INPUT)
			| DIO_MASK (DIO_FUNC_IO2, DIO_INPUT)
			| DIO_MASK (DIO_FUNC_IO3, DIO_INPUT)
			| DIO_MASK (DIO_FUNC_IO4, DIO_INPUT)
			| DIO_MASK (DIO_FUNC_IO5, DIO_INPUT)
			| DIO_MASK (DIO_FUNC_IO6, DIO_INPUT));

	static const char Header[] = "\nIO6 IO5 IO4 IO3 IO2 IO1 IO0\n";
	m_Screen.Write (Header, sizeof Header-1);

	while (1)
	{
		u8 ucInputs;
		m_DIO.GetAllInputs (&ucInputs);

		CString Msg ("\r");
		for (unsigned nFunc = DIO_FUNC_MAX; nFunc <= DIO_FUNC_MAX; nFunc--)
		{
			Msg.Append (ucInputs & DIO_MASK (nFunc, 1) ? " 1  " : " 0  ");
		}

		m_Screen.Write ((const char *) Msg, Msg.GetLength ());

		m_Timer.MsDelay (100);
	}
#else

#define REF_VOLTAGE	5.0

	// reading analog inputs
	if (m_DIO.GetVersion () < 12)
	{
		m_Logger.Write (FromKernel, LogPanic, "Analog input is not supported by dio version");
	}

	// See: http://www.bitwizard.nl/wiki/index.php/DIO_protocol#Using_the_analog_inputs
	m_DIO.SetAnalogChannels (1);
	m_DIO.CoupleAnalogChannel (0, DIO_ANALOG_IO0);
	m_DIO.SetAnalogSamples (256, 4);

	while (1)
	{
		unsigned nValueRaw, nValue;
		m_DIO.GetAnalogValueRaw (0, &nValueRaw);
		m_DIO.GetAnalogValue (0, &nValue);

		CString Msg;
		Msg.Format ("%4.2fV %5.3fV\n",
			    (double) nValueRaw * REF_VOLTAGE / DIO_ANALOG_VALUE_MAX,
			    (double) nValue    * REF_VOLTAGE / DIO_ANALOG_VALUE_MAX / 16.0);

		m_Screen.Write ((const char *) Msg, Msg.GetLength ());

		m_Timer.MsDelay (500);
	}
#endif

	return ShutdownHalt;
}
