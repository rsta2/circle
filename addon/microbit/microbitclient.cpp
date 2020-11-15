//
// microbitclient.cpp
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
#include <microbit/microbitclient.h>
#include <circle/devicenameservice.h>
#include <circle/sched/scheduler.h>
#include <circle/synchronize.h>
#include <circle/logger.h>
#include <circle/string.h>
#include <circle/util.h>
#include <assert.h>

static const char From[] = "microbit";

CMicrobitClient::CMicrobitClient (const char *pDeviceName)
:	m_DeviceName (pDeviceName),
	m_pMicrobit (0)
{
}

CMicrobitClient::~CMicrobitClient (void)
{
	m_pMicrobit = 0;
}

boolean CMicrobitClient::Initialize (void)
{
	assert (m_pMicrobit == 0);
	m_pMicrobit = (CUSBSerialDevice *)
		CDeviceNameService::Get ()->GetDevice (m_DeviceName, FALSE);
	if (m_pMicrobit == 0)
	{
		CLogger::Get ()->Write (From, LogError, "Device not found (\"%s\")",
					(const char *) m_DeviceName);

		return FALSE;
	}

	if (!m_pMicrobit->SetBaudRate (115200))
	{
		CLogger::Get ()->Write (From, LogError, "Cannot set baud rate");

		return FALSE;
	}

	return TRUE;
}

int CMicrobitClient::GetTemperature (void)
{
	if (!SendCommand ("!MI:TE\n"))
	{
		return MICROBIT_ERROR;
	}

	return ReceiveInteger ();
}

int CMicrobitClient::SetPixel (unsigned nPosX, unsigned nPosY, unsigned nBrightness)
{
	CString Cmd;
	Cmd.Format ("!DI:SP:%u:%u:%u\n", nPosX, nPosY, nBrightness);

	if (!SendCommand (Cmd))
	{
		return MICROBIT_ERROR;
	}

	return CheckStatus ();
}

int CMicrobitClient::ClearDisplay (void)
{
	if (!SendCommand ("!DI:CL\n"))
	{
		return MICROBIT_ERROR;
	}

	return CheckStatus ();
}

int CMicrobitClient::Show (const char *pString)
{
	assert (pString != 0);

	CString Cmd;
	Cmd.Format ("!DI:SH:%s\n", pString);

	if (!SendCommand (Cmd))
	{
		return MICROBIT_ERROR;
	}

	return CheckStatus ();
}

int CMicrobitClient::Scroll (const char *pString)
{
	assert (pString != 0);

	CString Cmd;
	Cmd.Format ("!DI:SC:%s\n", pString);

	if (!SendCommand (Cmd))
	{
		return MICROBIT_ERROR;
	}

	return CheckStatus ();
}

int CMicrobitClient::SetDisplay (boolean bOn)
{
	if (!SendCommand (bOn ? "!DI:ON\n" : "!DI:OF\n"))
	{
		return MICROBIT_ERROR;
	}

	return CheckStatus ();
}

int CMicrobitClient::GetLightLevel (void)
{
	if (!SendCommand ("!DI:LL\n"))
	{
		return MICROBIT_ERROR;
	}

	return ReceiveInteger ();
}

int CMicrobitClient::IsPressed (char chButton)
{
	assert (chButton == 'A' || chButton == 'B');

	if (!SendCommand (chButton == 'A' ? "!BU:IS:A\n" : "!BU:IS:B\n"))
	{
		return MICROBIT_ERROR;
	}

	return ReceiveInteger ();
}

int CMicrobitClient::WasPressed (char chButton)
{
	assert (chButton == 'A' || chButton == 'B');

	if (!SendCommand (chButton == 'A' ? "!BU:WA:A\n" : "!BU:WA:B\n"))
	{
		return MICROBIT_ERROR;
	}

	return ReceiveInteger ();
}

int CMicrobitClient::GetPresses (char chButton)
{
	assert (chButton == 'A' || chButton == 'B');

	if (!SendCommand (chButton == 'A' ? "!BU:GE:A\n" : "!BU:GE:B\n"))
	{
		return MICROBIT_ERROR;
	}

	return ReceiveInteger ();
}

int CMicrobitClient::ReadDigital (unsigned nPin)
{
	CString Cmd;
	Cmd.Format ("!PI:RD:%u\n", nPin);

	if (!SendCommand (Cmd))
	{
		return MICROBIT_ERROR;
	}

	return ReceiveInteger ();
}

int CMicrobitClient::WriteDigital (unsigned nPin, unsigned nValue)
{
	CString Cmd;
	Cmd.Format ("!PI:WD:%u:%u\n", nPin, nValue);

	if (!SendCommand (Cmd))
	{
		return MICROBIT_ERROR;
	}

	return CheckStatus ();
}

int CMicrobitClient::ReadAnalog (unsigned nPin)
{
	CString Cmd;
	Cmd.Format ("!PI:RA:%u\n", nPin);

	if (!SendCommand (Cmd))
	{
		return MICROBIT_ERROR;
	}

	return ReceiveInteger ();
}

int CMicrobitClient::WriteAnalog (unsigned nPin, unsigned nValue)
{
	CString Cmd;
	Cmd.Format ("!PI:WA:%u:%u\n", nPin, nValue);

	if (!SendCommand (Cmd))
	{
		return MICROBIT_ERROR;
	}

	return CheckStatus ();
}

int CMicrobitClient::SetAnalogPeriod (unsigned nPin, unsigned nMillis)
{
	CString Cmd;
	Cmd.Format ("!PI:PM:%u:%u\n", nPin, nMillis);

	if (!SendCommand (Cmd))
	{
		return MICROBIT_ERROR;
	}

	return CheckStatus ();
}

int CMicrobitClient::SetAnalogPeriodMicroseconds (unsigned nPin, unsigned nMicros)
{
	CString Cmd;
	Cmd.Format ("!PI:PU:%u:%u\n", nPin, nMicros);

	if (!SendCommand (Cmd))
	{
		return MICROBIT_ERROR;
	}

	return CheckStatus ();
}

int CMicrobitClient::IsTouched (unsigned nPin)
{
	CString Cmd;
	Cmd.Format ("!PI:IT:%u\n", nPin);

	if (!SendCommand (Cmd))
	{
		return MICROBIT_ERROR;
	}

	return ReceiveInteger ();
}

int CMicrobitClient::GetAccelX (void)
{
	if (!SendCommand ("!AC:GX\n"))
	{
		return MICROBIT_ERROR;
	}

	return ReceiveInteger ();
}

int CMicrobitClient::GetAccelY (void)
{
	if (!SendCommand ("!AC:GY\n"))
	{
		return MICROBIT_ERROR;
	}

	return ReceiveInteger ();
}

int CMicrobitClient::GetAccelZ (void)
{
	if (!SendCommand ("!AC:GZ\n"))
	{
		return MICROBIT_ERROR;
	}

	return ReceiveInteger ();
}

int CMicrobitClient::GetCurrentGesture (CString *pResult)
{
	assert (pResult != 0);

	if (!SendCommand ("!AC:CG\n"))
	{
		return MICROBIT_ERROR;
	}

	if (!ReceiveResult (pResult))
	{
		return MICROBIT_ERROR;
	}

	return 0;
}

int CMicrobitClient::IsGesture (const char *pGesture)
{
	assert (pGesture != 0);

	CString Cmd;
	Cmd.Format ("!AC:IG:%s\n", pGesture);

	if (!SendCommand (Cmd))
	{
		return MICROBIT_ERROR;
	}

	return ReceiveInteger ();
}

int CMicrobitClient::WasGesture (const char *pGesture)
{
	assert (pGesture != 0);

	CString Cmd;
	Cmd.Format ("!AC:WG:%s\n", pGesture);

	if (!SendCommand (Cmd))
	{
		return MICROBIT_ERROR;
	}

	return ReceiveInteger ();
}

int CMicrobitClient::CalibrateCompass (void)
{
	if (!SendCommand ("!CO:CA\n"))
	{
		return MICROBIT_ERROR;
	}

	return CheckStatus ();
}

int CMicrobitClient::IsCompassCalibrated (void)
{
	if (!SendCommand ("!CO:IC\n"))
	{
		return MICROBIT_ERROR;
	}

	return ReceiveInteger ();
}

int CMicrobitClient::ClearCompassCalibration (void)
{
	if (!SendCommand ("!CO:CC\n"))
	{
		return MICROBIT_ERROR;
	}

	return CheckStatus ();
}

int CMicrobitClient::GetCompassX (void)
{
	if (!SendCommand ("!CO:GX\n"))
	{
		return MICROBIT_ERROR;
	}

	return ReceiveInteger ();
}

int CMicrobitClient::GetCompassY (void)
{
	if (!SendCommand ("!CO:GY\n"))
	{
		return MICROBIT_ERROR;
	}

	return ReceiveInteger ();
}

int CMicrobitClient::GetCompassZ (void)
{
	if (!SendCommand ("!CO:GZ\n"))
	{
		return MICROBIT_ERROR;
	}

	return ReceiveInteger ();
}

int CMicrobitClient::GetHeading (void)
{
	if (!SendCommand ("!CO:HE\n"))
	{
		return MICROBIT_ERROR;
	}

	return ReceiveInteger ();
}

int CMicrobitClient::GetFieldStrength (void)
{
	if (!SendCommand ("!CO:GF\n"))
	{
		return MICROBIT_ERROR;
	}

	return ReceiveInteger ();
}

boolean CMicrobitClient::SendCommand (const char *pCommand)
{
	assert (m_pMicrobit != 0);
	assert (pCommand != 0);

	int nResult = m_pMicrobit->Write (pCommand, strlen (pCommand));
	if (nResult < 0)
	{
		CLogger::Get ()->Write (From, LogError, "Write error");

		return FALSE;
	}

	return TRUE;
}

boolean CMicrobitClient::ReceiveResult (CString *pResult)
{
	assert (pResult != 0);
	*pResult = "";

	boolean bContinue = TRUE;
	while (bContinue)
	{
		char Buffer [20];
		int nResult = m_pMicrobit->Read (Buffer, sizeof Buffer-1);
		if (nResult > 0)
		{
			assert ((size_t) nResult < sizeof Buffer);
			Buffer[nResult] = '\0';

			bContinue = Buffer[nResult-1] != '\n';
			if (!bContinue)
			{
				Buffer[nResult-1] = '\0';
			}

			pResult->Append (Buffer);
		}
		else if (nResult < 0)
		{
			CLogger::Get ()->Write (From, LogError, "Read error");

			return FALSE;
		}
		else
		{
			if (CScheduler::IsActive ())
			{
				CScheduler::Get ()->Yield ();
			}
		}
	}

	return TRUE;
}

int CMicrobitClient::ReceiveInteger (void)
{
	CString Buffer;
	if (!ReceiveResult (&Buffer))
	{
		return MICROBIT_ERROR;
	}

	return ConvertInteger (Buffer);
}

int CMicrobitClient::CheckStatus (void)
{
	CString Buffer;
	if (!ReceiveResult (&Buffer))
	{
		return MICROBIT_ERROR;
	}

	if (Buffer.Compare ("OK") != 0)
	{
		return MICROBIT_ERROR;
	}

	return MICROBIT_OK;
}

int CMicrobitClient::ConvertInteger (const char *pString)
{
	assert (pString != 0);

	boolean bMinus = *pString == '-';
	if (bMinus)
	{
		pString++;
	}

	char *pEnd = 0;
	unsigned long ulResult = strtoul (pString, &pEnd, 10);
	if (   pEnd != 0
	    && *pEnd != '\0')
	{
		return MICROBIT_ERROR;
	}

	if (ulResult > -MICROBIT_ERROR)
	{
		return MICROBIT_ERROR;
	}

	int nResult = (int) ulResult;
	if (bMinus)
	{
		nResult = -nResult;
	}

	return nResult;
}
