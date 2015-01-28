//
// i2cechoserver.h
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
#ifndef _i2cechoserver_h
#define _i2cechoserver_h

#include "../i2cping.h"
#include <circle/i2cslave.h>
#include <circle/types.h>

class CI2CEchoServer : public CI2CSlave
{
public:
	CI2CEchoServer (u8 ucSlaveAddress);
	~CI2CEchoServer (void);

	void Run (void);

private:
	boolean ReceiveRequest (void);
	boolean SendResponse (void);

private:
	TEchoHeader m_Header;

	u8 m_Data[ECHO_MAX_DATA_LENGTH];
};

#endif
