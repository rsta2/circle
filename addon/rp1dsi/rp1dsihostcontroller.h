//
// rp1dsihostcontroller.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2026  R. Stange <rsta2@gmx.net>
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
#ifndef _rp1dsi_rp1dsihostcontroller_h
#define _rp1dsi_rp1dsihostcontroller_h

#include "drm_mipi_dsi.h"
#include "drmcompat.h"
#include <circle/interrupt.h>
#include <circle/gpioclock.h>
#include <circle/types.h>

class CRP1DSIHostController : public mipi_dsi_host  /// Driver for DSI output on Raspberry Pi RP1
{
public:
	CRP1DSIHostController (CInterruptSystem *pInterrupt, unsigned nDepth, unsigned nDisplay,
			       const drm_display_mode *pMode);
	~CRP1DSIHostController (void);

	boolean Initialize (void);

	boolean Start (void *pFrameBuffer, unsigned nPitch);

	typedef void TVBlankHandler (void *pParam);
	void RegisterVBlankHandler (TVBlankHandler *pHandler, void *pParam);

private:
	static const u8 DataLanes = 1;
	static const u32 LanePolarities = 0;	// for all lanes
	static const u8 VC = 0;			// channel

private:
	static ssize_t rp1dsi_host_transfer (mipi_dsi_host *host, const mipi_dsi_msg *msg);

	void rp1dsi_mipicfg_setup (void);

	int rp1dsi_dsi_setup (drm_display_mode const *mode);
	void rp1dsi_dsi_send (u32 hdr, int len, const u8 *buf, bool use_lpm, bool req_ack);
	int rp1dsi_dsi_recv (int len, u8 *buf);
	void rp1dsi_dsi_set_cmdmode (int mode);
	void rp1dsi_dsi_stop (void);

	void dphy_transaction (u8 test_code, u8 test_data);
	static u64 dphy_get_div (u32 refclk, u64 vco_freq, u32 *ptr_m, u32 *ptr_n);
	void dphy_set_hsfreqrange (u32 freq_mhz);
	u32 dphy_configure_pll (u32 refclk, u32 vco_freq);
	u32 dphy_init (u32 ref_freq, u32 vco_freq);
	unsigned long rp1dsi_refclk_freq (void);
	int rp1dsi_dpiclk_start (u32 byte_clock, unsigned int bpp, unsigned int lanes);
	void rp1dsi_dpiclk_stop (void);
	u32 get_colorcode (mipi_dsi_pixel_format fmt);

	unsigned rp1dsi_dma_read(unsigned reg);
	void rp1dsi_dma_write(unsigned reg, unsigned val);
	int rp1dsi_dma_busy(void);
	void rp1dsi_dma_setup(u32 in_format, mipi_dsi_pixel_format out_format,
			      drm_display_mode const *mode);
	void rp1dsi_dma_update(uintptr addr, u32 offset, u32 stride);
	void rp1dsi_dma_stop(void);
	void rp1dsi_dma_vblank_ctrl(int enable);
	static void rp1dsi_dma_isr (void *pParam);

private:
	struct hsfreq_range
	{
		u16 mhz_max;
		u8  hsfreqrange;
		u8  clk_lp2hs;
		u8  clk_hs2lp;
		u8  data_lp2hs;		// excluding clk lane entry
		u8  data_hs2lp;
	};

private:
	CInterruptSystem *m_pInterrupt;
	unsigned m_nDepth;
	unsigned m_nDisplay;
	const drm_display_mode *m_pMode;

	uintptr m_ulDMABase;
	uintptr m_ulDSIBase;
	uintptr m_ulCFGBase;

	CGPIOClock m_CFGClock;
	CGPIOClock m_DPIClock;

	unsigned long m_display_flags;
	mipi_dsi_pixel_format m_display_format;

	u8 m_hsfreq_index;		// DPHY

	boolean m_bIRQConnected;
	boolean m_bDSIRunning;
	volatile boolean m_bDMARunning;

	TVBlankHandler *m_pVBlankHandler;
	void *m_pVBlankParam;

	static const hsfreq_range s_hsfreq_table[];

	static mipi_dsi_host_ops s_host_ops;
};

#endif
