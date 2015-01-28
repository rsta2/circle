//
// i2cechoserver.cpp
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
#include "i2cechoserver.h"
#include <circle/logger.h>
#include <circle/debug.h>
#include <assert.h>

const char FromI2CEchoServer[] = "i2cecho";

CI2CEchoServer::CI2CEchoServer (u8 ucSlaveAddress)
:	CI2CSlave (ucSlaveAddress)
{
}

CI2CEchoServer::~CI2CEchoServer (void)
{
}

void CI2CEchoServer::Run (void)
{
	while (1)
	{
		if (ReceiveRequest ())
		{
			SendResponse ();
		}
	}
}

boolean CI2CEchoServer::ReceiveRequest (void)
{
	if (Read (&m_Header, sizeof m_Header) != (int) sizeof m_Header)
	{
		CLogger::Get ()->Write (FromI2CEchoServer, LogWarning, "I2C read error (header)");

		return FALSE;
	}

	if (m_Header.ucPID != ECHO_PID_REQUEST)
	{
		CLogger::Get ()->Write (FromI2CEchoServer, LogWarning, "Invalid protocol ID received");

		return FALSE;
	}

	if (   m_Header.usDataLength == 0
	    || m_Header.usDataLength > ECHO_MAX_DATA_LENGTH)
	{
		CLogger::Get ()->Write (FromI2CEchoServer, LogWarning, "Invalid data length received");

		return FALSE;
	}

	if (Read (m_Data, m_Header.usDataLength) != (int) m_Header.usDataLength)
	{
		CLogger::Get ()->Write (FromI2CEchoServer, LogWarning, "I2C read error (data)");

		return FALSE;
	}

	CLogger::Get ()->Write (FromI2CEchoServer, LogNotice, "%u bytes received (ID %u)", (unsigned) m_Header.usDataLength, (unsigned) m_Header.ucID);

#ifndef NDEBUG
	//debug_hexdump (m_Data, m_Header.usDataLength, FromI2CEchoServer);
#endif

	return TRUE;
}

boolean CI2CEchoServer::SendResponse (void)
{
	m_Header.ucPID = ECHO_PID_RESPONSE;

	if (Write (&m_Header, sizeof m_Header) != (int) sizeof m_Header)
	{
		CLogger::Get ()->Write (FromI2CEchoServer, LogWarning, "I2C write error (header)");

		return FALSE;
	}

	if (Write (m_Data, m_Header.usDataLength) != (int) m_Header.usDataLength)
	{
		CLogger::Get ()->Write (FromI2CEchoServer, LogWarning, "I2C write error (data)");

		return FALSE;
	}

	CLogger::Get ()->Write (FromI2CEchoServer, LogNotice, "%u bytes sent (ID %u)", (unsigned) m_Header.usDataLength, (unsigned) m_Header.ucID);

	return TRUE;
}
