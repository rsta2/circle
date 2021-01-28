//
// bcm54213.h
//
// This driver has been ported from the Linux drivers:
//	Broadcom GENET (Gigabit Ethernet) controller driver
//	Broadcom UniMAC MDIO bus controller driver
//	Copyright (c) 2014-2017 Broadcom
//	Licensed under GPLv2
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2019-2020  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_bcm54213_h
#define _circle_bcm54213_h

#include <circle/netdevice.h>
#include <circle/macaddress.h>
#include <circle/timer.h>
#include <circle/spinlock.h>
#include <circle/types.h>

#define GENET_DESC_INDEX	16	// max. 16 priority queues and 1 default queue

struct TGEnetCB				// buffer control block
{
	uintptr		bd_addr;	// address of HW buffer descriptor
	u8		*buffer;	// pointer to frame buffer (DMA address)
};

struct TGEnetTxRing			// ring of Tx buffers
{
	unsigned	index;		// ring index
	unsigned	queue;		// queue index
	TGEnetCB	*cbs;		// tx ring buffer control block*/
	unsigned	size;		// size of each tx ring
	unsigned	clean_ptr;      // Tx ring clean pointer
	unsigned	c_index;	// last consumer index of each ring*/
	unsigned	free_bds;	// # of free bds for each ring
	unsigned	write_ptr;	// Tx ring write pointer SW copy
	unsigned	prod_index;	// Tx ring producer index SW copy
	unsigned	cb_ptr;		// Tx ring initial CB ptr
	unsigned	end_ptr;	// Tx ring end CB ptr
	void		(*int_enable)(TGEnetTxRing *);
};

struct TGEnetRxRing			// ring of Rx buffers
{
	unsigned	index;		// Rx ring index
	TGEnetCB	*cbs;		// Rx ring buffer control block
	unsigned	size;		// Rx ring size
	unsigned	c_index;	// Rx last consumer index
	unsigned	read_ptr;	// Rx ring read pointer
	unsigned	cb_ptr;		// Rx ring initial CB ptr
	unsigned	end_ptr;	// Rx ring end CB ptr
	unsigned	old_discards;
 	void		(*int_enable)(TGEnetRxRing *);
};

class CBcm54213Device : public CNetDevice	/// Driver for BCM54213PE Gigabit Ethernet Transceiver
{
public:
	CBcm54213Device (void);
	~CBcm54213Device (void);

	boolean Initialize (void);

	const CMACAddress *GetMACAddress (void) const;

	// returns TRUE if TX ring has currently free buffers
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
	// UMAC
	void reset_umac(void);
	void umac_reset2(void);
	void init_umac(void);
	void umac_enable_set(u32 mask, bool enable);

	// interrupt disable/enable
	void intr_disable(void);
	void enable_tx_intr(void);
	void enable_rx_intr(void);
	void link_intr_enable(void);

	static void tx_ring16_int_enable(TGEnetTxRing *ring);
	static void tx_ring_int_enable(TGEnetTxRing *ring);
	static void rx_ring16_int_enable(TGEnetRxRing *ring);

	// address and mode setting
	int set_hw_addr(void);
	void set_mdf_addr(unsigned char *addr, int *i, int *mc);
	void set_rx_mode(void);

	// HW filter block
	void hfb_init(void);

	// net enable and start
	void netif_start(void);

	// DMA initialization
	int init_dma(void);
	u32 dma_disable(void);
	void enable_dma(u32 dma_ctrl);

	// Tx queues, rings and buffers
	void init_tx_queues (bool enable);
	void init_tx_ring(unsigned index, unsigned size, unsigned start_ptr, unsigned end_ptr);
	TGEnetCB *get_txcb(TGEnetTxRing *ring);
	unsigned tx_reclaim(TGEnetTxRing *ring);
	void free_tx_cb(TGEnetCB *cb);

	// Rx queues, rings and buffers
	int init_rx_queues(void);
	int init_rx_ring(unsigned index, unsigned size, unsigned start_ptr, unsigned end_ptr);
	int alloc_rx_buffers(TGEnetRxRing *ring);
	void free_rx_buffers(void);
	u8 *rx_refill(TGEnetCB *cb);
	u8 *free_rx_cb(TGEnetCB *cb);

	// Helpers
	void dmadesc_set(uintptr d, u8 *addr, u32 value);
	void dmadesc_set_addr(uintptr d, u8 *addr);
	void dmadesc_set_length_status(uintptr d, u32 value);

	void udelay (unsigned nMicroSeconds);

	// interrupt handlers
	void InterruptHandler0 (void);
	void InterruptHandler1 (void);

	static void InterruptStub0 (void *pParam);
	static void InterruptStub1 (void *pParam);

	// MII
	int mii_probe(void);
	void mii_setup(void);
	int mii_config(bool init);

	// UniMAC MDIO
	int mdio_reset(void);
	int mdio_read(int reg);
	void mdio_write(int reg, u16 value);
	void mdio_wait(void);

	// PHY device
	int phy_read_status(void);

private:
	CTimer *m_pTimer;
	CMACAddress m_MACAddress;
	boolean m_bInterruptConnected;

	TGEnetCB *m_tx_cbs;				// Tx control blocks
	TGEnetTxRing m_tx_rings[GENET_DESC_INDEX+1];	// Tx rings

	TGEnetCB *m_rx_cbs;				// Rx control blocks
	TGEnetRxRing m_rx_rings[GENET_DESC_INDEX+1];	// Rx rings

	boolean m_crc_fwd_en;		// has FCS to be removed?

	// PHY status
	int m_phy_id;			// probed address of this PHY

	int m_link;			// 1: link is up
	int m_speed;			// 10, 100, 1000
	int m_duplex;			// 1: full duplex
	int m_pause;			// 1: pause capability

	int m_old_link;
	int m_old_speed;
	int m_old_duplex;
	int m_old_pause;

	CSpinLock m_TxSpinLock;
};

#endif
