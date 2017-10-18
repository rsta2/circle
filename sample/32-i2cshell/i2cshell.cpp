//
// i2cshell.cpp
//
// I2C slave detection method by Arjan van Vught <info@raspberrypi-dmx.nl>
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
#include "i2cshell.h"
#include <circle/machineinfo.h>
#include <circle/util.h>
#include <circle/timer.h>
#include <circle/stdarg.h>
#include <assert.h>

#define SLAVE_ADDRESS_MIN	0x03
#define SLAVE_ADDRESS_MAX	0x77

#define CLOCK_KHZ_MIN		5
#define CLOCK_KHZ_DEFAULT	100
#define CLOCK_KHZ_MAX		2000


const char CI2CShell::HelpMsg[] =
	"\n"
	"Command\t\tDescription\t\t\t\tAlias\n"
	"\n"
	"slave ADDRESS\tSet slave address (0x03-0x77)\n"
	"clock KHZ\tSelect clock rate in KHz\n"
	"detect [r|w]\tRead (default) or write detection\n"
	"read COUNT\tRead and display COUNT bytes\t\trd\n"
	"write BYTE...\tWrite byte(s)\t\t\t\twr\n"
	"delay MS\tDelay MS milliseconds\n"
	"reboot\t\tReboot the system\n"
	"\n"
	"WARNING: The write detection test may corrupt some devices (e.g. EEPROMs).\n"
	"\t The read detection test may lock some write-only devices.\n"
	"\t DO NOT USE THE DETECT COMMAND IF YOU ARE UNSURE!\n"
	"\n"
	"Numerical parameters can be given decimal or in hex (with \"0x\" prefix).\n"
	"Multiple commands can be entered on one line.\n"
	"\n";

CI2CShell::CI2CShell (CConsole *pConsole, CI2CMaster *pI2CMaster)
:	m_pConsole (pConsole),
	m_pI2CMaster (pI2CMaster),
	m_bContinue (TRUE),
	m_nI2CClockHz (CLOCK_KHZ_DEFAULT * 1000),
	m_ucSlaveAddress (INVALID_SLAVE)
{
}

CI2CShell::~CI2CShell (void)
{
	m_pConsole = 0;
	m_pI2CMaster = 0;
}

void CI2CShell::Run (void)
{
	Print ("\n\nI2C Shell\n"
	       "Using master #%u\n"
	       "Default clock rate is %u KHz\n"
	       "Enter \"help\" for help!\n\n",
	       CMachineInfo::Get ()->GetDevice (DeviceI2CMaster),
	       m_nI2CClockHz / 1000);

	while (m_bContinue)
	{
		ReadLine ();

		CString Command;
		while (GetToken (&Command))
		{
			if (((const char *) Command)[0] == '#')
			{
				break;
			}
			else if (Command.Compare ("slave") == 0)
			{
				if (!Slave ())
				{
					break;
				}
			}
			else if (Command.Compare ("clock") == 0)
			{
				if (!Clock ())
				{
					break;
				}
			}
			else if (Command.Compare ("detect") == 0)
			{
				if (!Detect ())
				{
					break;
				}
			}
			else if (   Command.Compare ("read") == 0
				 || Command.Compare ("rd") == 0)
			{
				if (!Read ())
				{
					break;
				}
			}
			else if (   Command.Compare ("write") == 0
				 || Command.Compare ("wr") == 0)
			{
				if (!Write ())
				{
					break;
				}
			}
			else if (Command.Compare ("delay") == 0)
			{
				if (!Delay ())
				{
					break;
				}
			}
			else if (Command.Compare ("reboot") == 0)
			{
				m_bContinue = FALSE;
			}
			else if (Command.Compare ("help") == 0)
			{
				Print (HelpMsg);
			}
			else
			{
				Print ("Unknown command: %s\n", (const char *) Command);
				break;
			}
		}
	}
}

boolean CI2CShell::Slave (void)
{
	unsigned nAddress = GetNumber ("Slave address", SLAVE_ADDRESS_MIN, SLAVE_ADDRESS_MAX);
	if (nAddress == INVALID_NUMBER)
	{
		return FALSE;
	}

	m_ucSlaveAddress = (u8) nAddress;

	return TRUE;
}

boolean CI2CShell::Clock (void)
{
	unsigned nClockKHz = GetNumber ("Clock rate", CLOCK_KHZ_MIN, CLOCK_KHZ_MAX);
	if (nClockKHz == INVALID_NUMBER)
	{
		return FALSE;
	}

	m_nI2CClockHz = nClockKHz * 1000;

	return TRUE;
}

boolean CI2CShell::Detect (void)
{
	boolean bWriteMode = FALSE;

	CString Mode;
	if (GetToken (&Mode))
	{
		if (Mode.Compare ("r") == 0)
		{
			// nothing to do
		}
		else if (Mode.Compare ("w") == 0)
		{
			bWriteMode = TRUE;
		}
		else
		{
			UnGetToken (Mode);
		}
	}

	assert (m_pI2CMaster != 0);
	m_pI2CMaster->SetClock (m_nI2CClockHz);

	u8 ucFirstSlaveAddress = INVALID_SLAVE;

	for (u8 ucAddress = SLAVE_ADDRESS_MIN; ucAddress <= SLAVE_ADDRESS_MAX; ucAddress++)
	{
		boolean bPresent = FALSE;

		if (bWriteMode)
		{
			if (m_pI2CMaster->Write (ucAddress, 0, 0) == 0)
			{
				bPresent = TRUE;
			}
		}
		else
		{
			u8 Buffer[1];
			if (m_pI2CMaster->Read (ucAddress, Buffer, sizeof Buffer) >= 0)
			{
				bPresent = TRUE;
			}
		}

		if (bPresent)
		{
			if (ucFirstSlaveAddress == INVALID_SLAVE)
			{
				Print ("Slave(s) at address:");

				ucFirstSlaveAddress = ucAddress;
			}

			Print (" 0x%02X", (unsigned) ucAddress);
		}
	}

	if (ucFirstSlaveAddress == INVALID_SLAVE)
	{
		Print ("No slave present\n");

		m_ucSlaveAddress = INVALID_SLAVE;

		return FALSE;
	}

	Print ("\n");

	m_ucSlaveAddress = ucFirstSlaveAddress;

	return TRUE;
}

boolean CI2CShell::Read (void)
{
	u8 Buffer[256];

	unsigned nCount = GetNumber ("Byte count", 1, sizeof Buffer);
	if (nCount == INVALID_NUMBER)
	{
		return FALSE;
	}

	if (m_ucSlaveAddress == INVALID_SLAVE)
	{
		Print ("Slave address not set\n");
		return FALSE;
	}

	assert (m_pI2CMaster != 0);
	m_pI2CMaster->SetClock (m_nI2CClockHz);

	int nResult = m_pI2CMaster->Read (m_ucSlaveAddress, Buffer, nCount);
	if (nResult <= 0)
	{
		PrintI2CError (nResult);
		return FALSE;
	}

	Print ("Data:");

	for (int i = 0; i < nResult; i++)
	{
		Print (" 0x%02X", (unsigned) Buffer[i]);
	}

	Print ("\n");

	return TRUE;
}

boolean CI2CShell::Write (void)
{
	u8 Buffer[256];

	CString Data;
	unsigned nCount;
	for (nCount = 0; nCount < sizeof Buffer && GetToken (&Data); nCount++)
	{
		unsigned nData = ConvertNumber (Data);
		if (nData == INVALID_NUMBER)
		{
			UnGetToken (Data);
			break;
		}

		if (nData > 255)
		{
			Print ("Data byte out of range: %s\n", (const char *) Data);
			return FALSE;
		}

		Buffer[nCount] = (u8) nData;
	}

	if (nCount == 0)
	{
		Print ("Data byte expected\n");
		return FALSE;
	}

	if (m_ucSlaveAddress == INVALID_SLAVE)
	{
		Print ("Slave address not set\n");
		return FALSE;
	}

	assert (m_pI2CMaster != 0);
	m_pI2CMaster->SetClock (m_nI2CClockHz);

	int nResult = m_pI2CMaster->Write (m_ucSlaveAddress, Buffer, nCount);
	if (nResult <= 0)
	{
		PrintI2CError (nResult);
		return FALSE;
	}

	return TRUE;
}

boolean CI2CShell::Delay (void)
{
	unsigned nMillis = GetNumber ("Delay", 1, 5000);
	if (nMillis == INVALID_NUMBER)
	{
		return FALSE;
	}

	CTimer::Get ()->MsDelay (nMillis);

	return TRUE;
}

unsigned CI2CShell::GetNumber (const char *pName, unsigned nMinimum, unsigned nMaximum)
{
	assert (pName != 0);

	CString Token;
	if (!GetToken (&Token))
	{
		Print ("%s expected\n", pName);
		return INVALID_NUMBER;
	}

	unsigned nValue = ConvertNumber (Token);
	if (nValue == INVALID_NUMBER)
	{
		Print ("Invalid number: %s\n", (const char *) Token);
		return INVALID_NUMBER;
	}

	assert (nMinimum < nMaximum);
	if (   nValue < nMinimum
	    || nValue > nMaximum)
	{
		Print ("%s out of range: %s\n", pName, (const char *) Token);
		return INVALID_NUMBER;
	}

	return nValue;
}

void CI2CShell::PrintI2CError (int nStatus)
{
	switch (nStatus)
	{
	case -I2C_MASTER_INALID_PARM:	Print ("Invalid parameter");		break;
	case -I2C_MASTER_ERROR_NACK:	Print ("Negative ACK\n");		break;
	case -I2C_MASTER_ERROR_CLKT:	Print ("Clock stretch timeout\n");	break;
	case -I2C_MASTER_DATA_LEFT:	Print ("Short transfer\n");		break;
	case 0:				Print ("No data\n");			break;
	default:			Print ("Unknown I2C error\n");		break;
	}
}

void CI2CShell::ReadLine (void)
{
	int nResult;

	do
	{
		if (m_ucSlaveAddress == INVALID_SLAVE)
		{
			Print ("I2C %u> ", m_nI2CClockHz/1000);
		}
		else
		{
			Print ("I2C %u 0x%02X> ", m_nI2CClockHz/1000, (unsigned) m_ucSlaveAddress);
		}

		assert (m_pConsole != 0);
		while ((nResult = m_pConsole->Read (m_LineBuffer, sizeof m_LineBuffer-1)) <= 0)
		{
			// just repeat
		}

		assert (nResult < (int) sizeof m_LineBuffer);
		assert (m_LineBuffer[nResult-1] == '\n');
		m_LineBuffer[nResult-1] = '\0';
	}
	while (nResult <= 1);

	m_bFirstToken = TRUE;
	m_UnGetToken = "";
}

boolean CI2CShell::GetToken (CString *pString)
{
	assert (pString != 0);

	if (m_UnGetToken.GetLength () > 0)
	{
		*pString = m_UnGetToken;
		m_UnGetToken = "";

		return TRUE;
	}

	char *p = strtok_r (m_bFirstToken ? m_LineBuffer : 0, " \t\n", &m_pSavePtr);
	m_bFirstToken = FALSE;

	if (p == 0)
	{
		return FALSE;
	}

	*pString = p;

	return TRUE;
}

void CI2CShell::UnGetToken (const CString &rString)
{
	assert (!m_bFirstToken);

	m_UnGetToken = rString;
}

unsigned CI2CShell::ConvertNumber (const CString &rString)
{
	const char *p = rString;

	int nBase = 10;
	if (    p[0] == '0'
	    && (p[1] == 'x' || p[1] == 'X'))
	{
		nBase = 16;
		p += 2;
	}

	char *pEndPtr = 0;
	unsigned long ulNumber = strtoul (p, &pEndPtr, nBase);
	if (    pEndPtr != 0
	    && *pEndPtr != '\0')
	{
		return INVALID_NUMBER;
	}

	assert (ulNumber <= (unsigned) -1);
	return (unsigned) ulNumber;
}

void CI2CShell::Print (const char *pFormat, ...)
{
	assert (pFormat != 0);

	va_list var;
	va_start (var, pFormat);

	CString Message;
	Message.FormatV (pFormat, var);

	assert (m_pConsole != 0);
	m_pConsole->Write (Message, Message.GetLength ());

	va_end (var);
}
