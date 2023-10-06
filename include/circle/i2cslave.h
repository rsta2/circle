//
// i2cslave.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2023  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_i2cslave_h
#define _circle_i2cslave_h

#include <circle/gpiopin.h>
#include <circle/types.h>

class CI2CSlave		/// Driver for I2C slave device
{
public:
	static const unsigned TimeoutNone    = 0;	///< Return immediately
	static const unsigned TimeoutForEver = -1;	///< Timeout never occurs

public:
	/// \param ucAddress 7-bit device address
	CI2CSlave (u8 ucAddress);

	~CI2CSlave (void);

	/// \return Operation successful?
	boolean Initialize (void);

	/// \param pBuffer Pointer to data buffer
	/// \param nCount Number of bytes to be read
	/// \param nTimeout_us Return after given number of us, when less than nCount bytes read
	/// \return Number of read bytes or < 0 on failure\n
	///	    When a timeout occurs, the result is smaller than nCount (or 0).
	/// \note Broadcasts to the General Call Address 0 will not be received.
	int Read (void *pBuffer, unsigned nCount, unsigned nTimeout_us = TimeoutForEver);

	/// \param pBuffer Pointer to data buffer
	/// \param nCount Number of bytes to be written
	/// \param nTimeout_us Return after given number of us, when less than nCount bytes written
	/// \return Number of written bytes or < 0 on failure\n
	///	    When a timeout occurs, the result is smaller than nCount (or 0).
	int Write (const void *pBuffer, unsigned nCount, unsigned nTimeout_us = TimeoutForEver);

private:
	u8 m_ucAddress;

	CGPIOPin m_SDA;
	CGPIOPin m_SCL;
};

#endif
