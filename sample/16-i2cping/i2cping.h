//
// i2cping.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
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
#ifndef _i2cping_h
#define _i2cping_h

#include <circle/types.h>
#include <circle/macros.h>

#define I2C_MASTER_DEVICE	1		// 0 on Raspberry Pi 1 Rev. 1 boards, 1 otherwise
#define I2C_MASTER_CONFIG	0		// 0 or 1 on Raspberry Pi 4, 0 otherwise
#define I2C_FAST_MODE		FALSE		// standard mode (100 Kbps) or fast mode (400 Kbps)

#define I2C_SLAVE_ADDRESS	20		// 7 bit slave address

#define ECHO_DATA_LENGTH	64		// number of data bytes transfered

struct TEchoHeader
{
	u8	ucPID;
#define ECHO_PID_REQUEST	0xA1
#define ECHO_PID_RESPONSE	0xA2
	u8	ucID;
	u16	usDataLength;
#define ECHO_MAX_DATA_LENGTH	2000
}
PACKED;

#endif
