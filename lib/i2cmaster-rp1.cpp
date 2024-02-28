//
// i2cmaster-rp1.cpp
//
// This driver has been ported from Linux:
//	drivers/i2c/busses/i2c-designware-*
//	Synopsys DesignWare I2C adapter driver.
//	Based on the TI DAVINCI I2C adapter driver.
//	Copyright (C) 2006 Texas Instruments.
//	Copyright (C) 2007 MontaVista Software Inc.
//	Copyright (C) 2009 Provigent Ltd.
//	SPDX-License-Identifier: GPL-2.0-or-later
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
#include <circle/i2cmaster.h>
#include <circle/memio.h>
#include <circle/timer.h>
#include <circle/macros.h>
#include <assert.h>

#define CLOCK_SYS_RATE_KHZ	200000

#define DEVICES			4
#define CONFIGS			3

#define GPIOS			2
	#define GPIO_SDA	0
	#define GPIO_SCL	1

// Adapter registers
#define DW_IC_CON		0x00
	#define DW_IC_CON_MASTER		BIT(0)
	#define DW_IC_CON_SPEED_STD		(1 << 1)
	#define DW_IC_CON_SPEED_FAST		(2 << 1)
	#define DW_IC_CON_SPEED_MASK		(3 << 1)
	#define DW_IC_CON_10BITADDR_MASTER	BIT(4)
	#define DW_IC_CON_RESTART_EN		BIT(5)
	#define DW_IC_CON_SLAVE_DISABLE		BIT(6)
	#define DW_IC_CON_STOP_DET_IFADDRESSED	BIT(7)
	#define DW_IC_CON_TX_EMPTY_CTRL		BIT(8)
	#define DW_IC_CON_RX_FIFO_FULL_HLD_CTRL	BIT(9)
#define DW_IC_TAR		0x04
	#define DW_IC_TAR_SPECIAL		BIT(11)
	#define DW_IC_TAR_10BITADDR_MASTER	BIT(12)
	#define DW_IC_TAR_SMBUS_QUICK_CMD	BIT(16)
#define DW_IC_SAR		0x08
#define DW_IC_DATA_CMD		0x10
	#define DW_IC_DATA_CMD_DAT		0xFF
#define DW_IC_SS_SCL_HCNT	0x14
#define DW_IC_SS_SCL_LCNT	0x18
#define DW_IC_FS_SCL_HCNT	0x1c
#define DW_IC_FS_SCL_LCNT	0x20
#define DW_IC_HS_SCL_HCNT	0x24
#define DW_IC_HS_SCL_LCNT	0x28
#define DW_IC_INTR_STAT		0x2c
	#define DW_IC_INTR_RX_UNDER		BIT(0)
	#define DW_IC_INTR_RX_OVER		BIT(1)
	#define DW_IC_INTR_RX_FULL		BIT(2)
	#define DW_IC_INTR_TX_OVER		BIT(3)
	#define DW_IC_INTR_TX_EMPTY		BIT(4)
	#define DW_IC_INTR_RD_REQ		BIT(5)
	#define DW_IC_INTR_TX_ABRT		BIT(6)
	#define DW_IC_INTR_RX_DONE		BIT(7)
	#define DW_IC_INTR_ACTIVITY		BIT(8)
	#define DW_IC_INTR_STOP_DET		BIT(9)
	#define DW_IC_INTR_START_DET		BIT(10)
	#define DW_IC_INTR_GEN_CALL		BIT(11)
	#define DW_IC_INTR_RESTART_DET		BIT(12)
	#define DW_IC_INTR_DEFAULT_MASK		(DW_IC_INTR_RX_FULL | \
						 DW_IC_INTR_TX_ABRT | \
						 DW_IC_INTR_STOP_DET)
	#define DW_IC_INTR_MASTER_MASK		(DW_IC_INTR_DEFAULT_MASK | \
						 DW_IC_INTR_TX_EMPTY)
#define DW_IC_INTR_MASK		0x30
#define DW_IC_RAW_INTR_STAT	0x34
#define DW_IC_RX_TL		0x38
#define DW_IC_TX_TL		0x3c
#define DW_IC_CLR_INTR		0x40
#define DW_IC_CLR_RX_UNDER	0x44
#define DW_IC_CLR_RX_OVER	0x48
#define DW_IC_CLR_TX_OVER	0x4c
#define DW_IC_CLR_RD_REQ	0x50
#define DW_IC_CLR_TX_ABRT	0x54
#define DW_IC_CLR_RX_DONE	0x58
#define DW_IC_CLR_ACTIVITY	0x5c
#define DW_IC_CLR_STOP_DET	0x60
#define DW_IC_CLR_START_DET	0x64
#define DW_IC_CLR_GEN_CALL	0x68
#define DW_IC_ENABLE		0x6c
#define DW_IC_STATUS		0x70
	#define DW_IC_STATUS_ACTIVITY		BIT(0)
#define DW_IC_TXFLR		0x74
#define DW_IC_RXFLR		0x78
#define DW_IC_SDA_HOLD		0x7c
	#define DW_IC_SDA_HOLD_RX_SHIFT		16
	#define DW_IC_SDA_HOLD_RX_MASK		(0xFF << 16)
#define DW_IC_TX_ABRT_SOURCE	0x80
#define DW_IC_ENABLE_STATUS	0x9c
#define DW_IC_CLR_RESTART_DET	0xa8
#define DW_IC_COMP_PARAM_1	0xf4
#define DW_IC_COMP_VERSION	0xf8
#define DW_IC_SDA_HOLD_MIN_VERS	0x3131312A
#define DW_IC_COMP_TYPE		0xfc
#define DW_IC_COMP_TYPE_VALUE	0x44570140

#define FIFO_SIZE		32

#define MAX_STANDARD_MODE_FREQ	100000
#define MAX_FAST_MODE_FREQ	400000
#define MAX_FAST_MODE_PLUS_FREQ	1000000

#define MICRO			1000000

#define DIV_ROUND_CLOSEST(x, d)	(((x) + (d)/2) / d)

static uintptr s_BaseAddress[DEVICES] =
{
	0x1F00070000UL,
	0x1F00074000UL,
	0x1F00078000UL,
	0x1F0007C000UL
};

#define NONE	{10000, 10000}

static unsigned s_GPIOConfig[DEVICES][CONFIGS][GPIOS] =
{
	// SDA, SCL
	{{ 0,  1}, { 8,  9},   NONE }, // ALT3
	{{ 2,  3}, {10, 11},   NONE }, // ALT3
	{{ 4,  5}, {12, 13},   NONE }, // ALT3
	{{ 6,  7}, {14, 15}, {22, 23}} // ALT3
};

#define ALT_FUNC(device, config)	GPIOModeAlternateFunction3

CI2CMaster::CI2CMaster (unsigned nDevice, boolean bFastMode, unsigned nConfig)
:	m_nDevice (nDevice),
	m_nBaseAddress (0),
	m_bFastMode (bFastMode),
	m_nConfig (nConfig),
	m_bValid (FALSE),
	m_ss_hcnt (0),
	m_ss_lcnt (0),
	m_fs_hcnt (0),
	m_fs_lcnt (0),
	m_fp_hcnt (0),
	m_fp_lcnt (0),
	m_sda_hold_time (0),
	m_BusTimings {0, 0, 0, 0, 0},
	m_SpinLock (TASK_LEVEL)
{
	if (   m_nDevice >= DEVICES
	    || m_nConfig >= CONFIGS
	    || s_GPIOConfig[nDevice][nConfig][0] >= GPIO_PINS)
	{
		return;
	}

	m_nBaseAddress = s_BaseAddress[nDevice];
	assert (m_nBaseAddress != 0);

	m_SDA.AssignPin (s_GPIOConfig[nDevice][nConfig][GPIO_SDA]);
	m_SDA.SetMode (ALT_FUNC (nDevice, nConfig));
	// See: RP1 peripherals, section 3.5.1.3
	m_SDA.SetPullMode (GPIOPullModeUp);
	m_SDA.SetSlewRate (GPIOSlewRateLimited);
	m_SDA.SetSchmittTrigger (TRUE);

	m_SCL.AssignPin (s_GPIOConfig[nDevice][nConfig][GPIO_SCL]);
	m_SCL.SetMode (ALT_FUNC (nDevice, nConfig));
	// See: RP1 peripherals, section 3.5.1.3
	m_SCL.SetPullMode (GPIOPullModeUp);
	m_SCL.SetSlewRate (GPIOSlewRateLimited);
	m_SCL.SetSchmittTrigger (TRUE);

	m_bValid = TRUE;
}

CI2CMaster::~CI2CMaster (void)
{
	if (m_bValid)
	{
		DisableAdapter ();

		m_SDA.SetMode (GPIOModeInput);
		m_SCL.SetMode (GPIOModeInput);

		m_bValid = FALSE;
	}

	m_nBaseAddress = 0;
}

boolean CI2CMaster::Initialize (void)
{
	if (!m_bValid)
	{
		return FALSE;
	}

	if (read32 (m_nBaseAddress + DW_IC_COMP_TYPE) != DW_IC_COMP_TYPE_VALUE)
	{
		return FALSE;
	}

	if (!DisableAdapter ())
	{
		return FALSE;
	}

	SetClock (m_bFastMode ? MAX_FAST_MODE_FREQ : MAX_STANDARD_MODE_FREQ);

#ifndef NDEBUG
	u32 param = read32 (m_nBaseAddress + DW_IC_COMP_PARAM_1);
	u32 tx_fifo_depth = ((param >> 16) & 0xff) + 1;
	u32 rx_fifo_depth = ((param >> 8)  & 0xff) + 1;
	assert (tx_fifo_depth == FIFO_SIZE);
	assert (rx_fifo_depth == FIFO_SIZE);
#endif

	/* Configure Tx/Rx FIFO threshold levels */
	write32 (m_nBaseAddress + DW_IC_TX_TL, FIFO_SIZE / 2);
	write32 (m_nBaseAddress + DW_IC_RX_TL, 0);

	write32 (m_nBaseAddress + DW_IC_INTR_MASK, 0);		// Disable interrupt

	return TRUE;
}

void CI2CMaster::SetClock (unsigned nClockSpeed)
{
	if (m_BusTimings.bus_freq_hz == nClockSpeed)
	{
		return;
	}

	if (nClockSpeed > MAX_FAST_MODE_PLUS_FREQ)
	{
		nClockSpeed = MAX_FAST_MODE_PLUS_FREQ;
	}

	set_timings (nClockSpeed);

	m_sda_hold_time = DIV_ROUND_CLOSEST (CLOCK_SYS_RATE_KHZ * m_BusTimings.sda_hold_ns, MICRO);

	set_timings_master ();

	/* Write standard speed timing parameters */
	write32 (m_nBaseAddress + DW_IC_SS_SCL_HCNT, m_ss_hcnt);
	write32 (m_nBaseAddress + DW_IC_SS_SCL_LCNT, m_ss_lcnt);

	/* Write fast mode/fast mode plus timing parameters */
	write32 (m_nBaseAddress + DW_IC_FS_SCL_HCNT, m_fs_hcnt);
	write32 (m_nBaseAddress + DW_IC_FS_SCL_LCNT, m_fs_lcnt);

	/* Write SDA hold time if supported */
	if (m_sda_hold_time)
	{
		write32 (m_nBaseAddress + DW_IC_SDA_HOLD, m_sda_hold_time);
	}

	/* Configure the I2C master */
	u32 master_cfg = DW_IC_CON_MASTER | DW_IC_CON_SLAVE_DISABLE | DW_IC_CON_RESTART_EN;
	if (m_BusTimings.bus_freq_hz == MAX_STANDARD_MODE_FREQ)
	{
		master_cfg |= DW_IC_CON_SPEED_STD;
	}
	else
	{
		master_cfg |= DW_IC_CON_SPEED_FAST;
	}
	write32 (m_nBaseAddress + DW_IC_CON, master_cfg);
}

int CI2CMaster::Read (u8 ucAddress, void *pBuffer, unsigned nCount)
{
	m_SpinLock.Acquire ();

	int nResult = Transfer (TRUE, ucAddress, pBuffer, nCount);

	m_SpinLock.Release ();

	return nResult;
}

int CI2CMaster::Write (u8 ucAddress, const void *pBuffer, unsigned nCount)
{
	m_SpinLock.Acquire ();

	int nResult = Transfer (FALSE, ucAddress, const_cast<void *> (pBuffer), nCount);

	m_SpinLock.Release ();

	return nResult;
}

boolean CI2CMaster::DisableAdapter (void)
{
	int timeout = 100;

	do
	{
		write32 (m_nBaseAddress + DW_IC_ENABLE, 0);

		/*
		 * The enable status register may be unimplemented, but
		 * in that case this test reads zero and exits the loop.
		 */
		u32 status = read32 (m_nBaseAddress + DW_IC_ENABLE_STATUS);
		if ((status & 1) == 0)
		{
			return TRUE;
		}

		/*
		 * Wait 10 times the signaling period of the highest I2C
		 * transfer supported by the driver (for 400KHz this is
		 * 25us) as described in the DesignWare I2C databook.
		 */
		CTimer::SimpleusDelay (25);
	}
	while (timeout--);

	return FALSE;
}

int CI2CMaster::Transfer (boolean bRead, u8 ucAddress, void *pBuffer, unsigned nCount)
{
	m_bRead = bRead;
	m_pRXBufferPtr = m_pTXBufferPtr = static_cast<u8 *> (pBuffer);
	m_nRXBufferLen = m_nTXBufferLen = nCount;
	m_nRXOutstanding = 0;
	m_nResult = 0;

	int ret = wait_bus_not_busy ();
	if (ret)
	{
		return ret;
	}

	xfer_init (ucAddress, nCount);

	unsigned nStartTicks = CTimer::GetClockTicks ();
	while (1)
	{
		if (CTimer::GetClockTicks () - nStartTicks >= CLOCKHZ/2)
		{
			// TODO: Recover bus

			m_nResult = -I2C_MASTER_TIMEOUT;

			break;
		}

		u32 abort_source;
		u32 stat = read_clear_intrbits (&abort_source);

		if (stat & DW_IC_INTR_TX_ABRT)
		{
			m_nResult = -I2C_MASTER_ERROR_NACK;	// TODO: Check abort_source
			m_nRXOutstanding = 0;

			write32 (m_nBaseAddress + DW_IC_INTR_MASK, 0);

			break;
		}

		if (stat & DW_IC_INTR_RX_FULL)
		{
			read ();
		}

		if (stat & DW_IC_INTR_TX_EMPTY)
		{
			xfer_msg ();
		}

		if (   (   (stat & (DW_IC_INTR_TX_ABRT | DW_IC_INTR_STOP_DET))
			|| m_nResult)
		    && (m_nRXOutstanding == 0))
		{
			break;
		}
	}

	write32 (m_nBaseAddress + DW_IC_ENABLE, 0);

	if (m_nResult)
	{
		return m_nResult;
	}

	if (m_bRead)
	{
		assert (nCount >= m_nRXBufferLen);
		nCount -= m_nRXBufferLen;
	}
	else
	{
		assert (nCount >= m_nTXBufferLen);
		nCount -= m_nTXBufferLen;
	}

	return nCount;
}

int CI2CMaster::wait_bus_not_busy (void)
{
	unsigned nStartTicks = CTimer::GetClockTicks ();
	while (read32 (m_nBaseAddress + DW_IC_STATUS) & DW_IC_STATUS_ACTIVITY)
	{
		CTimer::SimpleusDelay (1100);

		if (CTimer::GetClockTicks () - nStartTicks >= CLOCKHZ/50)
		{
			// TODO: Recover bus

			return -I2C_MASTER_BUS_BUSY;
		}
	}

	return 0;
}

void CI2CMaster::xfer_init (u8 ucAddress, unsigned nCount)
{
	/* Disable the adapter */
	write32 (m_nBaseAddress + DW_IC_ENABLE, 0);

	/* Convert a zero-length read into an SMBUS quick command */
	int ic_tar = 0;
	if (!nCount)
	{
		ic_tar = DW_IC_TAR_SPECIAL | DW_IC_TAR_SMBUS_QUICK_CMD;
	}

	// TODO: Support 10-bit address
	write32 (m_nBaseAddress + DW_IC_CON,   read32 (m_nBaseAddress + DW_IC_CON)
					     & ~DW_IC_CON_10BITADDR_MASTER);

	/* Set the slave (target) address */
	write32 (m_nBaseAddress + DW_IC_TAR, ucAddress | ic_tar);

	/* Enforce disabled interrupts (due to HW issues) */
	write32 (m_nBaseAddress + DW_IC_INTR_MASK, 0);

	/* Enable the adapter */
	write32 (m_nBaseAddress + DW_IC_ENABLE, 1);

	/* Dummy read to avoid the register getting stuck on Bay Trail */
	// read32 (m_nBaseAddress + DW_IC_ENABLE_STATUS);

	/* Clear and enable interrupts */
	read32 (m_nBaseAddress + DW_IC_CLR_INTR);
	write32 (m_nBaseAddress + DW_IC_INTR_MASK, DW_IC_INTR_MASTER_MASK);
}

u32 CI2CMaster::read_clear_intrbits (u32 *pAbortSource)
{
	assert (pAbortSource);

	/*
	 * The IC_INTR_STAT register just indicates "enabled" interrupts.
	 * The unmasked raw version of interrupt status bits is available
	 * in the IC_RAW_INTR_STAT register.
	 */
	u32 stat = read32 (m_nBaseAddress + DW_IC_INTR_STAT);

	/*
	 * Do not use the IC_CLR_INTR register to clear interrupts, or
	 * you'll miss some interrupts, triggered during the period from
	 * readl(IC_INTR_STAT) to readl(IC_CLR_INTR).
	 *
	 * Instead, use the separately-prepared IC_CLR_* registers.
	 */
	if (stat & DW_IC_INTR_RX_UNDER)
		read32 (m_nBaseAddress + DW_IC_CLR_RX_UNDER);
	if (stat & DW_IC_INTR_RX_OVER)
		read32 (m_nBaseAddress + DW_IC_CLR_RX_OVER);
	if (stat & DW_IC_INTR_TX_OVER)
		read32 (m_nBaseAddress + DW_IC_CLR_TX_OVER);
	if (stat & DW_IC_INTR_RD_REQ)
		read32 (m_nBaseAddress + DW_IC_CLR_RD_REQ);
	if (stat & DW_IC_INTR_TX_ABRT)
	{
		/*
		 * The IC_TX_ABRT_SOURCE register is cleared whenever
		 * the IC_CLR_TX_ABRT is read. Preserve it beforehand.
		 */
		*pAbortSource = read32 (m_nBaseAddress + DW_IC_TX_ABRT_SOURCE);
		read32 (m_nBaseAddress + DW_IC_CLR_TX_ABRT);
	}
	if (stat & DW_IC_INTR_RX_DONE)
		read32 (m_nBaseAddress + DW_IC_CLR_RX_DONE);
	if (stat & DW_IC_INTR_ACTIVITY)
		read32 (m_nBaseAddress + DW_IC_CLR_ACTIVITY);
	if (   (stat & DW_IC_INTR_STOP_DET)
	    && (   (m_nRXOutstanding == 0)
	        || (stat & DW_IC_INTR_RX_FULL)))
		read32 (m_nBaseAddress + DW_IC_CLR_STOP_DET);
	if (stat & DW_IC_INTR_START_DET)
		read32 (m_nBaseAddress + DW_IC_CLR_START_DET);
	if (stat & DW_IC_INTR_GEN_CALL)
		read32 (m_nBaseAddress + DW_IC_CLR_GEN_CALL);

	return stat;
}

void CI2CMaster::read (void)
{
	if (!m_bRead)
	{
		return;
	}

	u32 len = m_nRXBufferLen;
	u8 *buf = m_pRXBufferPtr;

	u32 rx_valid = read32 (m_nBaseAddress + DW_IC_RXFLR);

	for (; len > 0 && rx_valid > 0; len--, rx_valid--)
	{
		u32 tmp = read32 (m_nBaseAddress + DW_IC_DATA_CMD);
		tmp &= DW_IC_DATA_CMD_DAT;
		*buf++ = tmp;

		m_nRXOutstanding--;
	}

	m_nRXBufferLen = len;
	m_pRXBufferPtr = buf;
}

void CI2CMaster::xfer_msg (void)
{
	u32 intr_mask = DW_IC_INTR_MASTER_MASK;

	u32 buf_len = m_nTXBufferLen;
	u8 *buf = m_pTXBufferPtr;

	u32 flr = read32 (m_nBaseAddress + DW_IC_TXFLR);
	int tx_limit = FIFO_SIZE - flr;
	flr = read32 (m_nBaseAddress + DW_IC_RXFLR);
	int rx_limit = FIFO_SIZE - flr;

	/* Handle SMBUS quick commands */
	if (!buf_len)
	{
		write32 (m_nBaseAddress + DW_IC_DATA_CMD, m_bRead ? 0x300 : 0x200);
	}

	while (buf_len > 0 && tx_limit > 0 && rx_limit > 0)
	{
		u32 cmd = 0;
		if (buf_len == 1)
		{
			cmd |= BIT(9);
		}

		if (m_bRead) {
			/* Avoid rx buffer overrun */
			if (m_nRXOutstanding >= FIFO_SIZE)
			{
				break;
			}

			write32 (m_nBaseAddress + DW_IC_DATA_CMD, cmd | 0x100);

			rx_limit--;
			m_nRXOutstanding++;
		}
		else
		{
			write32 (m_nBaseAddress + DW_IC_DATA_CMD, cmd | *buf++);
		}

		tx_limit--;
		buf_len--;
	}

	m_pTXBufferPtr = buf;
	m_nTXBufferLen = buf_len;

	if (!m_nTXBufferLen)
	{
		intr_mask &= ~DW_IC_INTR_TX_EMPTY;
	}

	if (m_nResult)
	{
		intr_mask = 0;
	}

	write32 (m_nBaseAddress + DW_IC_INTR_MASK, intr_mask);
}

void CI2CMaster::set_timings (u32 bus_freq_hz)
{
	m_BusTimings.bus_freq_hz = bus_freq_hz;

	m_BusTimings.scl_rise_ns =   m_BusTimings.bus_freq_hz <= MAX_STANDARD_MODE_FREQ
				   ? 1000
				   : (m_BusTimings.bus_freq_hz <= MAX_FAST_MODE_FREQ ? 300 : 120);

	m_BusTimings.scl_fall_ns = m_BusTimings.bus_freq_hz <= MAX_FAST_MODE_FREQ ? 300 : 120;

	m_BusTimings.sda_fall_ns = m_BusTimings.scl_fall_ns;

	m_BusTimings.sda_hold_ns = 0;
}

void CI2CMaster::set_timings_master (void)
{
	/* Set standard and fast speed dividers for high/low periods */
	u32 sda_falling_time = m_BusTimings.sda_fall_ns ?: 300; /* ns */
	u32 scl_falling_time = m_BusTimings.scl_fall_ns ?: 300; /* ns */

	/* Calculate SCL timing parameters for standard mode if not set */
	if (!m_ss_hcnt || !m_ss_lcnt)
	{
		m_ss_hcnt = scl_hcnt (CLOCK_SYS_RATE_KHZ,
				      4000,	/* tHD;STA = tHIGH = 4.0 us */
				      sda_falling_time,
				      0,	/* 0: DW default, 1: Ideal */
				      0);	/* No offset */
		m_ss_lcnt = scl_lcnt (CLOCK_SYS_RATE_KHZ,
				      4700,	/* tLOW = 4.7 us */
				      scl_falling_time,
				      0);	/* No offset */
	}

	/*
	 * Set SCL timing parameters for fast mode or fast mode plus. Only
	 * difference is the timing parameter values since the registers are
	 * the same.
	 */
	if (m_BusTimings.bus_freq_hz == MAX_FAST_MODE_PLUS_FREQ)
	{
		/*
		 * Check are Fast Mode Plus parameters available. Calculate
		 * SCL timing parameters for Fast Mode Plus if not set.
		 */
		if (m_fp_hcnt && m_fp_lcnt)
		{
			m_fs_hcnt = m_fp_hcnt;
			m_fs_lcnt = m_fp_lcnt;
		}
		else
		{
			m_fs_hcnt = scl_hcnt (CLOCK_SYS_RATE_KHZ,
					      260,	/* tHIGH = 260 ns */
					      sda_falling_time,
					      0,	/* DW default */
					      0);	/* No offset */
			m_fs_lcnt = scl_lcnt (CLOCK_SYS_RATE_KHZ,
					      500,	/* tLOW = 500 ns */
					      scl_falling_time,
					      0);	/* No offset */
		}
	}

	/* Calculate SCL timing parameters for fast mode if not set. */
	if (!m_fs_hcnt || !m_fs_lcnt)
	{
		m_fs_hcnt = scl_hcnt (CLOCK_SYS_RATE_KHZ,
				      600,	/* tHD;STA = tHIGH = 0.6 us */
				      sda_falling_time,
				      0,	/* 0: DW default, 1: Ideal */
				      0);	/* No offset */
		m_fs_lcnt = scl_lcnt (CLOCK_SYS_RATE_KHZ,
				      1300,	/* tLOW = 1.3 us */
				      scl_falling_time,
				      0);	/* No offset */
	}

	set_sda_hold ();
}

void CI2CMaster::set_sda_hold (void)
{
	/* Configure SDA Hold Time if required */
	u32 reg = read32 (m_nBaseAddress + DW_IC_COMP_VERSION);
	if (reg >= DW_IC_SDA_HOLD_MIN_VERS)
	{
		if (!m_sda_hold_time)
		{
			/* Keep previous hold time setting if no one set it */
			m_sda_hold_time = read32 (m_nBaseAddress + DW_IC_SDA_HOLD);
		}

		/*
		 * Workaround for avoiding TX arbitration lost in case I2C
		 * slave pulls SDA down "too quickly" after falling edge of
		 * SCL by enabling non-zero SDA RX hold. Specification says it
		 * extends incoming SDA low to high transition while SCL is
		 * high but it appears to help also above issue.
		 */
		if (!(m_sda_hold_time & DW_IC_SDA_HOLD_RX_MASK))
		{
			m_sda_hold_time |= 1 << DW_IC_SDA_HOLD_RX_SHIFT;
		}
	}
	else if (m_sda_hold_time)
	{
		m_sda_hold_time = 0;
	}
}

u32 CI2CMaster::scl_hcnt (u32 ic_clk, u32 tSYMBOL, u32 tf, int cond, int offset)
{
	/*
	 * DesignWare I2C core doesn't seem to have solid strategy to meet
	 * the tHD;STA timing spec.  Configuring _HCNT based on tHIGH spec
	 * will result in violation of the tHD;STA spec.
	 */
	if (cond)
		/*
		 * Conditional expression:
		 *
		 *   IC_[FS]S_SCL_HCNT + (1+4+3) >= IC_CLK * tHIGH
		 *
		 * This is based on the DW manuals, and represents an ideal
		 * configuration.  The resulting I2C bus speed will be
		 * faster than any of the others.
		 *
		 * If your hardware is free from tHD;STA issue, try this one.
		 */
		return DIV_ROUND_CLOSEST ((u64) ic_clk * tSYMBOL, MICRO) - 8 + offset;
	else
		/*
		 * Conditional expression:
		 *
		 *   IC_[FS]S_SCL_HCNT + 3 >= IC_CLK * (tHD;STA + tf)
		 *
		 * This is just experimental rule; the tHD;STA period turned
		 * out to be proportinal to (_HCNT + 3).  With this setting,
		 * we could meet both tHIGH and tHD;STA timing specs.
		 *
		 * If unsure, you'd better to take this alternative.
		 *
		 * The reason why we need to take into account "tf" here,
		 * is the same as described in i2c_dw_scl_lcnt().
		 */
		return DIV_ROUND_CLOSEST ((u64) ic_clk * (tSYMBOL + tf), MICRO) - 3 + offset;
}

u32 CI2CMaster::scl_lcnt (u32 ic_clk, u32 tLOW, u32 tf, int offset)
{
	/*
	 * Conditional expression:
	 *
	 *   IC_[FS]S_SCL_LCNT + 1 >= IC_CLK * (tLOW + tf)
	 *
	 * DW I2C core starts counting the SCL CNTs for the LOW period
	 * of the SCL clock (tLOW) as soon as it pulls the SCL line.
	 * In order to meet the tLOW timing spec, we need to take into
	 * account the fall time of SCL signal (tf).  Default tf value
	 * should be 0.3 us, for safety.
	 */
	return DIV_ROUND_CLOSEST ((u64) ic_clk * (tLOW + tf), MICRO) - 1 + offset;
}
