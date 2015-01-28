//
// i2cechoclient.h
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
#ifndef _i2cechoclient_h
#define _i2cechoclient_h

#include "../i2cping.h"
#include <circle/i2cmaster.h>
#include <circle/types.h>

class CI2CEchoClient
{
public:
	CI2CEchoClient (u8 ucSlaveAddress, CI2CMaster *pMaster);
	~CI2CEchoClient (void);

	boolean DoPing (u8  ucID,		// consecutive number of request
			u16 usDataLength);

private:
	boolean SendRequest (u8 ucID, u16 usDataLength);
	boolean ReceiveResponse (void);

private:
	u8 m_ucSlaveAddress;
	CI2CMaster *m_pMaster;
};

#endif
