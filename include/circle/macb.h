//
// macb.h
//
// This driver has been ported from the U-Boot driver:
//	drivers/net/macb.*
//	Copyright (C) 2005-2006 Atmel Corporation
//	SPDX-License-Identifier: GPL-2.0+
//
// Ported to Raspberry Pi 5 by aniplay62@GitHub
// Ported to Circle by R. Stange
//
#ifndef _circle_macb_h
#define _circle_macb_h

#include <circle/netdevice.h>
#include <circle/macaddress.h>
#include <circle/gpiopin.h>
#include <circle/types.h>

class CMACBDevice : public CNetDevice	/// Driver for MACB/GEM Ethernet NIC of Raspberry Pi 5
{
public:
	CMACBDevice (void);
	~CMACBDevice (void);

	boolean Initialize (void);

	const CMACAddress *GetMACAddress (void) const;

	// returns TRUE if TX has currently free buffers
	boolean IsSendFrameAdvisable (void);

	boolean SendFrame (const void *pBuffer, unsigned nLength);

	// pBuffer must have size FRAME_BUFFER_SIZE
	boolean ReceiveFrame (void *pBuffer, unsigned *pResultLength);

	// returns TRUE if PHY link is up
	boolean IsLinkUp (void);

	TNetDeviceSpeed GetLinkSpeed (void);

	// update device settings according to PHY status
	boolean UpdatePHY (void);

private:
	struct TDMADescriptor
	{
		u32	addr;
		u32	ctrl;
		u32	addrh;
		u32	unused;
	};

	int eth_alloc (void);
	int macb_initialize (void);
	void macb_halt (void);

	u32 macb_dbw (void);
	u32 gem_mdc_clk_div (int id);
	void write_hwaddr (void);
	void set_addr (TDMADescriptor *desc, uintptr addr);
	void gmac_configure_dma (void);
	void gmac_init_multi_queues (void);

	int phy_init (void);
	void phy_setup (void);
	int phy_find (void);
	int phy_read_status (void);
	void phy_write_shadow (u16 shadow, u16 val);
	void phy_write_exp (u16 reg, u16 val);

	void mdio_write (u8 phy_adr, u8 reg, u16 value);
	u16 mdio_read (u8 phy_adr, u8 reg);
	void mdio_reset (void);

	static unsigned mii_nway_result (unsigned negotiated);

private:
	CMACAddress m_MACAddress;

	u8 *m_rx_buffer;
	u8 *m_tx_buffer;
	TDMADescriptor *m_rx_ring;
	TDMADescriptor *m_tx_ring;
	TDMADescriptor *m_dummy_desc;
	uintptr m_rx_buffer_dma;
	uintptr m_tx_buffer_dma;
	uintptr m_rx_ring_dma;
	uintptr m_tx_ring_dma;
	uintptr m_dummy_desc_dma;

	unsigned m_rx_tail;

	unsigned m_tx_head;
	unsigned m_tx_outstanding;
	unsigned m_tx_last_ticks;

	u16 m_phy_addr;
	int m_link;		// link state (1: up, 0: down)
	int m_speed;		// _10BASET, _100BASET, or _1000BASET
	int m_duplex;		// 1: full duplex
	int m_old_link;
	int m_old_speed;
	int m_old_duplex;

	CGPIOPin m_PHYResetPin;
};

#endif
