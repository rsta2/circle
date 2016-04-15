//
// spidio.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016  R. Stange <rsta2@o2online.de>
//
// See: http://www.bitwizard.nl/wiki/index.php/DIO_protocol
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
#include <dio/spidio.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <assert.h>

#define WRITE_CLOCK	200000
#define READ_CLOCK	20000

#define CS_HOLD_USEC	1

CSPIDIODevice::CSPIDIODevice (CSPIMaster *pSPIMaster, unsigned nChipSelect, u8 ucAddress)
:	m_pSPIMaster (pSPIMaster),
	m_nChipSelect (nChipSelect),
	m_ucAddress (ucAddress),
	m_nVersion (0)
{
}

CSPIDIODevice::~CSPIDIODevice (void)
{
	m_pSPIMaster = 0;
}

boolean CSPIDIODevice::Initialize (void)
{
	u8 Buffer[20] = {(u8) (m_ucAddress+1), 0x01};

	assert (m_pSPIMaster != 0);
	m_pSPIMaster->SetClock (READ_CLOCK);
	m_pSPIMaster->SetCSHoldTime (CS_HOLD_USEC);

	if (m_pSPIMaster->WriteRead (m_nChipSelect, Buffer, Buffer, sizeof Buffer) != sizeof Buffer)
	{
		return FALSE;
	}

	Buffer[sizeof Buffer-1] = '\0';
	m_nVersion = ConvertVersion ((const char *) Buffer+2);
	if (m_nVersion < 10)
	{
		CLogger::Get ()->Write ("spidio", LogError, "Device not found (CE%u)", m_nChipSelect);

		return FALSE;
	}

	return TRUE;
}

unsigned CSPIDIODevice::GetVersion (void) const
{
	return m_nVersion;
}

boolean CSPIDIODevice::SetModes (u8 ucModeMask)
{
	u8 Buffer[] = {m_ucAddress, 0x30, ucModeMask};

	assert (m_pSPIMaster != 0);
	m_pSPIMaster->SetClock (WRITE_CLOCK);
	m_pSPIMaster->SetCSHoldTime (CS_HOLD_USEC);

	if (m_pSPIMaster->Write (m_nChipSelect, Buffer, sizeof Buffer) != sizeof Buffer)
	{
		return FALSE;
	}

	return TRUE;
}

boolean CSPIDIODevice::SetAllOutputs (u8 ucMask)
{
	u8 Buffer[] = {m_ucAddress, 0x10, ucMask};

	assert (m_pSPIMaster != 0);
	m_pSPIMaster->SetClock (WRITE_CLOCK);
	m_pSPIMaster->SetCSHoldTime (CS_HOLD_USEC);

	return m_pSPIMaster->Write (m_nChipSelect, Buffer, sizeof Buffer) == sizeof Buffer;
}

boolean CSPIDIODevice::SetOutput (unsigned nFunction, unsigned nLevel)
{
	assert (nFunction <= DIO_FUNC_MAX);
	assert (nLevel <= DIO_HIGH);
	u8 Buffer[] = {m_ucAddress, (u8) (0x20 + nFunction), (u8) nLevel};

	assert (m_pSPIMaster != 0);
	m_pSPIMaster->SetClock (WRITE_CLOCK);
	m_pSPIMaster->SetCSHoldTime (CS_HOLD_USEC);

	return m_pSPIMaster->Write (m_nChipSelect, Buffer, sizeof Buffer) == sizeof Buffer;
}

boolean CSPIDIODevice::GetAllInputs (u8 *pMask)
{
	u8 Buffer[3] = {(u8) (m_ucAddress+1), 0x10};

	assert (m_pSPIMaster != 0);
	m_pSPIMaster->SetClock (READ_CLOCK);
	m_pSPIMaster->SetCSHoldTime (CS_HOLD_USEC);

	if (m_pSPIMaster->WriteRead (m_nChipSelect, Buffer, Buffer, sizeof Buffer) != sizeof Buffer)
	{
		return FALSE;
	}

	assert (pMask != 0);
	*pMask = Buffer[2];

	return TRUE;
}

boolean CSPIDIODevice::GetInput (unsigned nFunction, unsigned *pLevel)
{
	assert (nFunction <= DIO_FUNC_MAX);
	u8 Buffer[3] = {(u8) (m_ucAddress+1), (u8) (0x20 + nFunction)};

	assert (m_pSPIMaster != 0);
	m_pSPIMaster->SetClock (READ_CLOCK);
	m_pSPIMaster->SetCSHoldTime (CS_HOLD_USEC);

	if (m_pSPIMaster->WriteRead (m_nChipSelect, Buffer, Buffer, sizeof Buffer) != sizeof Buffer)
	{
		return FALSE;
	}

	assert (pLevel != 0);
	*pLevel = Buffer[2];

	return TRUE;
}

unsigned CSPIDIODevice::ConvertVersion (const char *pString)
{
	static const char ID[] = "spi_dio ";
	if (memcmp (pString, ID, sizeof ID-1) != 0)
	{
		return 0;
	}

	pString += sizeof ID-1;

	char *pEnd = 0;
	unsigned long ulMajor = strtoul (pString, &pEnd, 10);
	if (    pEnd == 0
	    || *pEnd != '.')
	{
		return 0;
	}

	pString = pEnd+1;
	pEnd = 0;
	unsigned long ulMinor = strtoul (pString, &pEnd, 10);
	if (    pEnd != 0
	    && *pEnd != '\0')
	{
		return 0;
	}

	if (ulMinor > 9)
	{
		ulMinor = 9;
	}

	return (unsigned) (ulMajor * 10 + ulMinor);
}
