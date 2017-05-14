//
// mcp7941x.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016  R. Stange <rsta2@o2online.de>
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
#ifndef _rtc_mcp7941x_h
#define _rtc_mcp7941x_h

#include <rtc/rtc.h>
#include <circle/i2cmaster.h>
#include <circle/time.h>
#include <circle/types.h>

class CMCP7941X : public CRealTimeClock
{
public:
	CMCP7941X (CI2CMaster *pI2CMaster, unsigned nI2CClockHz = 100000, u8 ucSlaveAddress = 0x6F);
	~CMCP7941X (void);

	boolean Initialize (void);

	boolean Get (CTime *pTime);
	boolean Set (const CTime &Time);

	// access battery buffered SRAM, ucAddress := 0..63
	boolean ReadSRAM (u8 ucAddress, void *pBuffer, unsigned nCount);
	boolean WriteSRAM (u8 ucAddress, const void *pBuffer, unsigned nCount);

private:
	CI2CMaster *m_pI2CMaster;
	unsigned    m_nI2CClockHz;
	u8	    m_ucSlaveAddress;
};

#endif
