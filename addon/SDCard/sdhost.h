//
// sdhost.h
//
// BCM2835 SD host driver.
//
// Author:	Phil Elwell <phil@raspberrypi.org>
//		Copyright (C) 2015-2016 Raspberry Pi (Trading) Ltd.
//
// Ported to Circle by R. Stange
//
// Based on
//  mmc-bcm2835.c by Gellert Weisz
// which is, in turn, based on
//  sdhci-bcm2708.c by Broadcom
//  sdhci-bcm2835.c by Stephen Warren and Oleksandr Tymoshenko
//  sdhci.c and sdhci-pci.c by Pierre Ossman
//
// This program is free software; you can redistribute it and/or modify it
// under the terms and conditions of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef _SDCard_sdhost_h
#define _SDCard_sdhost_h

#include <SDCard/mmchost.h>
#include <SDCard/mmc.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/gpiopin.h>
#include <circle/spinlock.h>
#include <circle/bcm2835.h>
#include <circle/memio.h>
#include <circle/types.h>

struct bcm2835_host
{
	mmc_host		*mmc;

	u32			pio_timeout;	/* In CLOCKHZ ticks */

	unsigned		clock;		/* Current clock speed */

	unsigned		max_clk;	/* Max possible freq */

//	tasklet_struct		finish_tasklet;	/* Tasklet structures */
//
//	work_struct		cmd_wait_wq;	/* Workqueue function */
//
//	timer_list		timer;		/* Timer for timeouts */

	sg_mapping_iter		sg_miter;	/* SG state for PIO */
	unsigned		blocks;		/* remaining PIO blocks */

	int			irq;		/* Device IRQ */

	u32			cmd_quick_poll_retries;
	u32			ns_per_fifo_word;

	/* cached registers */
	u32			hcfg;
	u32			cdiv;

	mmc_request		*mrq;			/* Current request */
	mmc_command		*cmd;			/* Current command */
	mmc_data		*data;			/* Current data request */
	unsigned		data_complete:1;	/* Data finished before cmd */

	unsigned		use_busy:1;		/* Wait for busy interrupt */

	unsigned		use_sbc:1;		/* Send CMD23 */

	unsigned		debug:1;		/* Enable debug output */
	unsigned		firmware_sets_cdiv:1;	/* Let the firmware manage the clock */
	unsigned		reset_clock:1;		/* Reset the clock fore the next request */

	int			max_delay;	/* maximum length of time spent waiting */
	unsigned		stop_time;	/* when the last stop was issued */
	u32			delay_after_stop; /* minimum time between stop and subsequent data transfer */
	u32			delay_after_this_stop; /* minimum time between this stop and subsequent data transfer */
	u32			user_overclock_50; /* User's preferred frequency to use when 50MHz is requested (in MHz) */
	u32			overclock_50;	/* frequency to use when 50MHz is requested (in MHz) */
	u32			overclock;	/* Current frequency if overclocked, else zero */

// 	u32			sectors;	/* Cached card size in sectors */
};

class CSDHOSTDevice : public CMMCHost
{
public:
	CSDHOSTDevice (CInterruptSystem	*pInterruptSystem, CTimer *pTimer);
	~CSDHOSTDevice (void);

	boolean Initialize (void);

	void Reset (void);
	void SetIOS (mmc_ios *pIOS);
	void Request (mmc_request *pRequest);

private:
	int set_sdhost_clock (u32 msg[3]);
	void dumpcmd (mmc_command *cmd, const char *label);
	void dumpregs (void);
	void set_power (boolean on);
	void reset_internal (void);
	void reset (mmc_host *mmc);
	void init (int soft);
	void wait_transfer_complete (void);
	void read_block_pio (void);
	void write_block_pio (void);
	void transfer_pio (void);
	void set_transfer_irqs (void);
	void prepare_data (mmc_command *cmd);
	boolean send_command (mmc_command *cmd);
	void finish_data (void);
	void finish_command (void);
	void transfer_complete (void);
// 	void timeout (timer_list *t);
	void busy_irq (u32 intmask);
	void data_irq (u32 intmask);
	void block_irq (u32 intmask);
	void irq_handler (void);
	static void irq_stub (void *param);
	void set_clock(unsigned clock);
	void request (mmc_host *mmc, mmc_request *mrq);
	void set_ios (mmc_host *mmc, mmc_ios *ios);
// 	void cmd_wait_work (work_struct *work);
	void tasklet_finish (void);
	int add_host(void);
	int probe (void);
	int remove (void);

	void write (u32 val, int reg)
	{
		write32 (ARM_SDHOST_BASE + reg, val);
	}

	u32 read (int reg)
	{
		return read32 (ARM_SDHOST_BASE + reg);
	}

private:
	CInterruptSystem *m_pInterruptSystem;
	CTimer		 *m_pTimer;

	CGPIOPin m_GPIO34_39[6];	// WiFi
	CGPIOPin m_GPIO48_53[6];	// SD card

	bcm2835_host m_Host;
	bcm2835_host *const host = &m_Host;

	CSpinLock m_SpinLock;
};

#endif
