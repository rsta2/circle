//
// i2cechoclient.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2015  R. Stange <rsta2@o2online.de>
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
#include "i2cechoclient.h"
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/debug.h>
#include <assert.h>

static const char FromI2CEchoClient[] = "i2cecho";

CI2CEchoClient::CI2CEchoClient (u8 ucSlaveAddress, CI2CMaster *pMaster)
:	m_ucSlaveAddress (ucSlaveAddress),
	m_pMaster (pMaster)
{
	assert (m_pMaster != 0);
}

CI2CEchoClient::~CI2CEchoClient (void)
{
	m_pMaster = 0;
}

boolean CI2CEchoClient::DoPing (u8 ucID, u16 usDataLength)
{
	if (!SendRequest (ucID, usDataLength))
	{
		return FALSE;
	}

	CTimer::Get ()->MsDelay (50);		// processing delay on echo server

	if (!ReceiveResponse ())
	{
		return FALSE;
	}

	return TRUE;
}

boolean CI2CEchoClient::SendRequest (u8 ucID, u16 usDataLength)
{
	assert (usDataLength > 0);
	assert (usDataLength <= ECHO_MAX_DATA_LENGTH);

	TEchoHeader Header;
	Header.ucPID = ECHO_PID_REQUEST;
	Header.ucID = ucID;
	Header.usDataLength = usDataLength;

	assert (m_pMaster != 0);
	if (m_pMaster->Write (m_ucSlaveAddress, &Header, sizeof Header) != (int) sizeof Header)
	{
		CLogger::Get ()->Write (FromI2CEchoClient, LogWarning, "I2C write error (header)");

		return FALSE;
	}

	u8 Data[usDataLength];
	for (unsigned i = 0; i < usDataLength; i++)
	{
		Data[i] = (u8) i;
	}

	if (m_pMaster->Write (m_ucSlaveAddress, Data, usDataLength) != (int) usDataLength)
	{
		CLogger::Get ()->Write (FromI2CEchoClient, LogWarning, "I2C write error (data)");

		return FALSE;
	}

	CLogger::Get ()->Write (FromI2CEchoClient, LogNotice, "%u bytes sent (ID %u)", (unsigned) usDataLength, (unsigned) ucID);

	return TRUE;
}

boolean CI2CEchoClient::ReceiveResponse (void)
{
	TEchoHeader Header;

	assert (m_pMaster != 0);
	if (m_pMaster->Read (m_ucSlaveAddress, &Header, sizeof Header) != (int) sizeof Header)
	{
		CLogger::Get ()->Write (FromI2CEchoClient, LogWarning, "I2C read error (header)");

		return FALSE;
	}

	if (Header.ucPID != ECHO_PID_RESPONSE)
	{
		CLogger::Get ()->Write (FromI2CEchoClient, LogWarning, "Invalid protocol ID received");

		return FALSE;
	}

	if (   Header.usDataLength == 0
	    || Header.usDataLength > ECHO_MAX_DATA_LENGTH)
	{
		CLogger::Get ()->Write (FromI2CEchoClient, LogWarning, "Invalid data length received");

		return FALSE;
	}
	
	CTimer::Get ()->usDelay (100);		// processing delay on echo server

	u8 Buffer[Header.usDataLength];
	if (m_pMaster->Read (m_ucSlaveAddress, Buffer, Header.usDataLength) != (int) Header.usDataLength)
	{
		CLogger::Get ()->Write (FromI2CEchoClient, LogWarning, "I2C read error (data)");

		return FALSE;
	}

	CLogger::Get ()->Write (FromI2CEchoClient, LogNotice, "%u bytes received (ID %u)", (unsigned) Header.usDataLength, (unsigned) Header.ucID);

#ifndef NDEBUG
	//debug_hexdump (Buffer, Header.usDataLength, FromI2CEchoClient);
#endif

	return TRUE;
}
