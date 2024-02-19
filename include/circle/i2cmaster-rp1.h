//
// i2cmaster-rp1.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2023  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_i2cmaster_rp1_h
#define _circle_i2cmaster_rp1_h

#ifndef _circle_i2cmaster_h
	#error Do not include this header file directly!
#endif

#include <circle/gpiopin.h>
#include <circle/spinlock.h>
#include <circle/types.h>

class CI2CMaster
{
public:
	CI2CMaster (unsigned nDevice, boolean bFastMode = FALSE, unsigned nConfig = 0);
	~CI2CMaster (void);

	boolean Initialize (void);

	void SetClock (unsigned nClockSpeed);

	int Read (u8 ucAddress, void *pBuffer, unsigned nCount);

	int Write (u8 ucAddress, const void *pBuffer, unsigned nCount);

private:
	boolean DisableAdapter (void);

	int Transfer (boolean bRead, u8 ucAddress, void *pBuffer, unsigned nCount);
	int wait_bus_not_busy (void);
	void xfer_init (u8 ucAddress, unsigned nCount);
	u32 read_clear_intrbits (u32 *pAbortSource);
	void read (void);
	void xfer_msg (void);

	void set_timings (u32 bus_freq_hz);
	void set_timings_master (void);
	void set_sda_hold (void);
	static u32 scl_hcnt (u32 ic_clk, u32 tSYMBOL, u32 tf, int cond, int offset);
	static u32 scl_lcnt (u32 ic_clk, u32 tLOW, u32 tf, int offset);

private:
	unsigned m_nDevice;
	uintptr  m_nBaseAddress;
	boolean  m_bFastMode;
	unsigned m_nConfig;
	boolean  m_bValid;

	CGPIOPin m_SDA;
	CGPIOPin m_SCL;

	// Transfer parameters
	boolean m_bRead;		// Is read operation (slave to master)?
	u8 *m_pRXBufferPtr;
	unsigned m_nRXBufferLen;
	unsigned m_nRXOutstanding;
	u8 *m_pTXBufferPtr;
	unsigned m_nTXBufferLen;
	int m_nResult;			// Error code

	// Timing parameters
	u16 m_ss_hcnt;
	u16 m_ss_lcnt;
	u16 m_fs_hcnt;
	u16 m_fs_lcnt;
	u16 m_fp_hcnt;
	u16 m_fp_lcnt;
	u32 m_sda_hold_time;

	struct
	{
		u32 bus_freq_hz;
		u32 scl_rise_ns;
		u32 scl_fall_ns;
		u32 sda_fall_ns;
		u32 sda_hold_ns;
	}
	m_BusTimings;

	CSpinLock m_SpinLock;
};

#endif
