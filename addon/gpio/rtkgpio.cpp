//
// rtkgpio.cpp
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
#include <circle/logger.h>
#include "rtkgpio.h"

static const char FromRtkgpio[] = "rtkgpio";

// Serial baudrate
#define RTKGPIO_BAUD_RATE		230400

// Define the lowest and highest GPIO pin numbers
#define RTKGPIO_PIN_MIN			0
#define RTKGPIO_PIN_MAX			27

CRTKGpioDevice::CRTKGpioDevice (CUSBSerialCH341Device *pCH341)
:	m_pCH341(pCH341)
{
	assert(m_pCH341 != 0);
}

CRTKGpioDevice::~CRTKGpioDevice ()
{
}

boolean CRTKGpioDevice::Initialize (void)
{
	assert(m_pCH341 != 0);

	if (!m_pCH341->SetBaudRate (RTKGPIO_BAUD_RATE))
	{
		return FALSE;
	}

	if (!GetVersion ())
	{
		return FALSE;
	}

	return TRUE;
}

boolean CRTKGpioDevice::GetVersion (void)
{
	if (m_pCH341->Write ("V?", 2) < 0)
	{
		CLogger::Get ()->Write (FromRtkgpio, LogError, "Write error V?");

		return FALSE;
	}

	u8 rxData[32] = {0};
	if (Read (rxData, 32) < 0)
	{
		CLogger::Get ()->Write (FromRtkgpio, LogError, "Read error version");

		return FALSE;
	}

	CLogger::Get ()->Write (FromRtkgpio, LogDebug, "firmware version: %s", rxData);

	return TRUE;
}

boolean CRTKGpioDevice::SetPinDirectionOutput (unsigned nPin)
{
	return SetPinDirection (nPin, RtkGpioDirectionOutput);
}

boolean CRTKGpioDevice::SetPinDirectionInput (unsigned nPin)
{
	return SetPinDirection (nPin, RtkGpioDirectionInput);
}

boolean CRTKGpioDevice::SetPinPullNone(unsigned nPin)
{
	return SetPinPull (nPin, RtkGpioPullNone);
}

boolean CRTKGpioDevice::SetPinPullDown (unsigned nPin)
{
	return SetPinPull (nPin, RtkGpioPullDown);
}

boolean CRTKGpioDevice::SetPinPullUp (unsigned nPin)
{
	return SetPinPull (nPin, RtkGpioPullUp);
}

boolean CRTKGpioDevice::SetPinLevelLow (unsigned nPin)
{
	return SetPinLevel (nPin, RtkGpioLevelLow);
}

boolean CRTKGpioDevice::SetPinLevelHigh (unsigned nPin)
{
	return SetPinLevel (nPin, RtkGpioLevelHigh);
}

boolean CRTKGpioDevice::SetPinPull (unsigned nPin, TRtkGpioPull ePull)
{
	assert(nPin >= RTKGPIO_PIN_MIN && nPin <= RTKGPIO_PIN_MAX);

	u8 cmd[3] = {0};
	cmd[0] = 'G';
	cmd[1] = (u8)(nPin + 'a');
	switch (ePull) {
	case RtkGpioPullNone:
		cmd[2] = 'N';
		break;
	case RtkGpioPullDown:
		cmd[2] = 'D';
		break;
	case RtkGpioPullUp:
		cmd[2] = 'U';
		break;
	}

	if (m_pCH341->Write (cmd, sizeof (cmd)) < 0)
	{
		CLogger::Get ()->Write (FromRtkgpio, LogError, "Failed to set pin %d pull", nPin);

		return FALSE;
	}
	//CLogger::Get ()->Write (FromRtkgpio, LogDebug, "Pin %d pull %s", nPin,
	//			ePull == RtkGpioPullNone ? "none" : (ePull == RtkGpioPullDown ? "down" : "up"));

	return TRUE;
}

boolean CRTKGpioDevice::SetPinDirection (unsigned nPin, TRtkGpioDirection eDirection)
{
	assert(nPin >= RTKGPIO_PIN_MIN && nPin <= RTKGPIO_PIN_MAX);

	u8 cmd[3] = {0};
	cmd[0] = 'G';
	cmd[1] = (u8)(nPin + 'a');
	switch (eDirection) {
	case RtkGpioDirectionOutput:
		cmd[2] = 'O';
		break;
	case RtkGpioDirectionInput:
		cmd[2] = 'I';
		break;
	}

	if (m_pCH341->Write (cmd, sizeof (cmd)) < 0)
	{
		CLogger::Get ()->Write (FromRtkgpio, LogError, "Failed to set pin %d direction", nPin);

		return FALSE;
	}
	//CLogger::Get ()->Write (FromRtkgpio, LogDebug, "Pin %d direction %s", nPin,
	//			eDirection == RtkGpioDirectionInput ? "input" : "output");

	return TRUE;
}

boolean CRTKGpioDevice::SetPinLevel (unsigned nPin, TRtkGpioLevel eLevel)
{
	assert(nPin >= RTKGPIO_PIN_MIN && nPin <= RTKGPIO_PIN_MAX);

	u8 cmd[3] = {0};
	cmd[0] = 'G';
	cmd[1] = (u8)(nPin + 'a');
	switch (eLevel) {
	case RtkGpioLevelLow:
		cmd[2] = '0';
		break;
	case RtkGpioLevelHigh:
		cmd[2] = '1';
		break;
	case RtkGpioLevelUnknown:
		CLogger::Get ()->Write (FromRtkgpio, LogError, "Invalid pin %d level", nPin);
		return FALSE;
		break;
	}

	if (m_pCH341->Write (cmd, sizeof (cmd)) < 0)
	{
		CLogger::Get ()->Write (FromRtkgpio, LogError, "Failed to set pin %d level", nPin);

		return FALSE;
	}
	//CLogger::Get ()->Write (FromRtkgpio, LogDebug, "Pin %d level %s", nPin,
	//			eLevel == RtkGpioLevelLow ? "low" : "high");

	return TRUE;
}

TRtkGpioLevel CRTKGpioDevice::GetPinLevel (unsigned nPin)
{
	assert(nPin >= RTKGPIO_PIN_MIN && nPin <= RTKGPIO_PIN_MAX);

	u8 cmd[3] = {0};
	cmd[0] = 'G';
	cmd[1] = (u8)(nPin + 'a');
	cmd[2] = '?';
	if (m_pCH341->Write (cmd, sizeof (cmd)) < 0)
	{
		CLogger::Get ()->Write (FromRtkgpio, LogError, "Failed to get pin %d level", nPin);

		return RtkGpioLevelUnknown;
	}

	u8 rxData[4] = {0};
	if (Read (rxData, 4) < 0)
	{
		CLogger::Get ()->Write (FromRtkgpio, LogError, "Read error");

		return RtkGpioLevelUnknown;
	}
	//CLogger::Get ()->Write (FromRtkgpio, LogDebug, "Read %s %2X %2X %2X %2X",
	//			cmd, rxData[0], rxData[1], rxData[2], rxData[3]);

	switch (rxData[1]) {
	case '0':
		//CLogger::Get ()->Write (FromRtkgpio, LogDebug, "Pin %d level low", nPin);
		return RtkGpioLevelLow;
		break;
	case '1':
		//CLogger::Get ()->Write (FromRtkgpio, LogDebug, "Pin %d level high", nPin);
		return RtkGpioLevelHigh;
		break;
	}

	CLogger::Get ()->Write (FromRtkgpio, LogError, "Invalid level");
	return RtkGpioLevelUnknown;
}

int CRTKGpioDevice::Read (void *pBuffer, size_t nCount)
{
	assert (m_pCH341 != 0);

	int nResult;
	do
	{
		nResult = m_pCH341->Read (pBuffer, nCount);
	}
	while (nResult == 0);

	return nResult;
}
