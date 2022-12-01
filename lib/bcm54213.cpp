//
// bcm54213.cpp
//
// Driver for BCM54213PE Gigabit Ethernet Transceiver of Raspberry Pi 4
//
// This driver has been ported from the Linux drivers:
//	Broadcom GENET (Gigabit Ethernet) controller driver
//	Broadcom UniMAC MDIO bus controller driver
//	Copyright (c) 2014-2017 Broadcom
//	Licensed under GPLv2
// The phy_read_status() routine is taken from Linux too:
//	drivers/net/phy/phy_device.c
//	Framework for finding and configuring PHYs.
//	Author: Andy Fleming
//	Copyright (c) 2004 Freescale Semiconductor, Inc.
//	Licensed under GPLv2
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2019-2021  R. Stange <rsta2@o2online.de>
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
#include <circle/bcm54213.h>
#include <circle/bcmpropertytags.h>
#include <circle/interrupt.h>
#include <circle/bcm2711int.h>
#include <circle/memio.h>
#include <circle/bcm2711.h>
#include <circle/synchronize.h>
#include <circle/logger.h>
#include <circle/string.h>
#include <circle/util.h>
#include <circle/macros.h>
#include <assert.h>

#define GENET_V5			5	// the only supported GENET version

// HW params for GENET_V5
#define TX_QUEUES			4
#define TX_BDS_PER_Q			32	// buffer descriptors per Tx queue
#define RX_QUEUES			0	// not implemented
#define RX_BDS_PER_Q			0	// buffer descriptors per Rx queue
#define HFB_FILTER_CNT			48
#define HFB_FILTER_SIZE			128
#define QTAG_MASK			0x3F
#define HFB_OFFSET			0x8000
#define HFB_REG_OFFSET			0xFC00
#define RDMA_OFFSET			0x2000
#define TDMA_OFFSET			0x4000
#define WORDS_PER_BD			3	// word per buffer descriptor

#define RX_BUF_LENGTH			2048

// DMA descriptors
#define TOTAL_DESC			256	// number of buffer descriptors (same for Rx/Tx)

#define DMA_DESC_LENGTH_STATUS		0x00	// in bytes of data in buffer
#define DMA_DESC_ADDRESS_LO		0x04	// lower bits of PA
#define DMA_DESC_ADDRESS_HI		0x08	// upper 32 bits of PA, GENETv4+

#define DMA_DESC_SIZE			(WORDS_PER_BD * sizeof(u32))

// queues
#define GENET_Q0_PRIORITY		0	// highest priority queue

						// default queues
#define GENET_Q16_RX_BD_CNT		(TOTAL_DESC - RX_QUEUES * RX_BDS_PER_Q)
#define GENET_Q16_TX_BD_CNT		(TOTAL_DESC - TX_QUEUES * TX_BDS_PER_Q)

#define TX_RING_INDEX			1	// using highest TX priority queue

// Tx/Rx DMA register offset, skip 256 descriptors
#define GENET_TDMA_REG_OFF		(TDMA_OFFSET + TOTAL_DESC * DMA_DESC_SIZE)
#define GENET_RDMA_REG_OFF		(RDMA_OFFSET + TOTAL_DESC * DMA_DESC_SIZE)

// DMA configuration
#define DMA_MAX_BURST_LENGTH		8

#define DMA_FC_THRESH_HI		(TOTAL_DESC >> 4)
#define DMA_FC_THRESH_LO		5

// Ethernet defaults
#define ETH_FCS_LEN			4
#define ETH_ZLEN			60
#define ENET_MAX_MTU_SIZE		1536	// with padding

// HW register offset and field definitions
#define UMAC_HD_BKP_CTRL		0x004
#define	 HD_FC_EN			(1 << 0)
#define  HD_FC_BKOFF_OK			(1 << 1)
#define  IPG_CONFIG_RX_SHIFT		2
#define  IPG_CONFIG_RX_MASK		0x1F

#define UMAC_CMD			0x008
#define  CMD_TX_EN			(1 << 0)
#define  CMD_RX_EN			(1 << 1)
#define  UMAC_SPEED_10			0
#define  UMAC_SPEED_100			1
#define  UMAC_SPEED_1000		2
#define  UMAC_SPEED_2500		3
#define  CMD_SPEED_SHIFT		2
#define  CMD_SPEED_MASK			3
#define  CMD_PROMISC			(1 << 4)
#define  CMD_PAD_EN			(1 << 5)
#define  CMD_CRC_FWD			(1 << 6)
#define  CMD_PAUSE_FWD			(1 << 7)
#define  CMD_RX_PAUSE_IGNORE		(1 << 8)
#define  CMD_TX_ADDR_INS		(1 << 9)
#define  CMD_HD_EN			(1 << 10)
#define  CMD_SW_RESET			(1 << 13)
#define  CMD_LCL_LOOP_EN		(1 << 15)
#define  CMD_AUTO_CONFIG		(1 << 22)
#define  CMD_CNTL_FRM_EN		(1 << 23)
#define  CMD_NO_LEN_CHK			(1 << 24)
#define  CMD_RMT_LOOP_EN		(1 << 25)
#define  CMD_PRBL_EN			(1 << 27)
#define  CMD_TX_PAUSE_IGNORE		(1 << 28)
#define  CMD_TX_RX_EN			(1 << 29)
#define  CMD_RUNT_FILTER_DIS		(1 << 30)

#define UMAC_MAC0			0x00C
#define UMAC_MAC1			0x010
#define UMAC_MAX_FRAME_LEN		0x014

#define UMAC_MODE			0x44
#define  MODE_LINK_STATUS		(1 << 5)

#define UMAC_EEE_CTRL			0x064
#define  EN_LPI_RX_PAUSE		(1 << 0)
#define  EN_LPI_TX_PFC			(1 << 1)
#define  EN_LPI_TX_PAUSE		(1 << 2)
#define  EEE_EN				(1 << 3)
#define  RX_FIFO_CHECK			(1 << 4)
#define  EEE_TX_CLK_DIS			(1 << 5)
#define  DIS_EEE_10M			(1 << 6)
#define  LP_IDLE_PREDICTION_MODE	(1 << 7)

#define UMAC_EEE_LPI_TIMER		0x068
#define UMAC_EEE_WAKE_TIMER		0x06C
#define UMAC_EEE_REF_COUNT		0x070
#define  EEE_REFERENCE_COUNT_MASK	0xFFFF

#define UMAC_TX_FLUSH			0x334

#define UMAC_MIB_START			0x400

#define UMAC_MDIO_CMD			0x614
#define  MDIO_START_BUSY		(1 << 29)
#define  MDIO_READ_FAIL			(1 << 28)
#define  MDIO_RD			(2 << 26)
#define  MDIO_WR			(1 << 26)
#define  MDIO_PMD_SHIFT			21
#define  MDIO_PMD_MASK			0x1F
#define  MDIO_REG_SHIFT			16
#define  MDIO_REG_MASK			0x1F

#define UMAC_RBUF_OVFL_CNT_V1		0x61C
#define RBUF_OVFL_CNT_V2		0x80
#define RBUF_OVFL_CNT_V3PLUS		0x94

#define UMAC_MPD_CTRL			0x620
#define  MPD_EN				(1 << 0)
#define  MPD_PW_EN			(1 << 27)
#define  MPD_MSEQ_LEN_SHIFT		16
#define  MPD_MSEQ_LEN_MASK		0xFF

#define UMAC_MPD_PW_MS			0x624
#define UMAC_MPD_PW_LS			0x628
#define UMAC_RBUF_ERR_CNT_V1		0x634
#define RBUF_ERR_CNT_V2			0x84
#define RBUF_ERR_CNT_V3PLUS		0x98
#define UMAC_MDF_ERR_CNT		0x638
#define UMAC_MDF_CTRL			0x650
#define UMAC_MDF_ADDR			0x654
#define UMAC_MIB_CTRL			0x580
#define  MIB_RESET_RX			(1 << 0)
#define  MIB_RESET_RUNT			(1 << 1)
#define  MIB_RESET_TX			(1 << 2)

#define RBUF_CTRL			0x00
#define  RBUF_64B_EN			(1 << 0)
#define  RBUF_ALIGN_2B			(1 << 1)
#define  RBUF_BAD_DIS			(1 << 2)

#define RBUF_STATUS			0x0C
#define  RBUF_STATUS_WOL		(1 << 0)
#define  RBUF_STATUS_MPD_INTR_ACTIVE	(1 << 1)
#define  RBUF_STATUS_ACPI_INTR_ACTIVE	(1 << 2)

#define RBUF_CHK_CTRL			0x14
#define  RBUF_RXCHK_EN			(1 << 0)
#define  RBUF_SKIP_FCS			(1 << 4)

#define RBUF_ENERGY_CTRL		0x9C
#define  RBUF_EEE_EN			(1 << 0)
#define  RBUF_PM_EN			(1 << 1)

#define RBUF_TBUF_SIZE_CTRL		0xB4

#define RBUF_HFB_CTRL_V1		0x38
#define  RBUF_HFB_FILTER_EN_SHIFT	16
#define  RBUF_HFB_FILTER_EN_MASK	0xFFFF0000
#define  RBUF_HFB_EN			(1 << 0)
#define  RBUF_HFB_256B			(1 << 1)
#define  RBUF_ACPI_EN			(1 << 2)

#define RBUF_HFB_LEN_V1			0x3C
#define  RBUF_FLTR_LEN_MASK		0xFF
#define  RBUF_FLTR_LEN_SHIFT		8

#define TBUF_CTRL			0x00
#define TBUF_BP_MC			0x0C
#define TBUF_ENERGY_CTRL		0x14
#define  TBUF_EEE_EN			(1 << 0)
#define  TBUF_PM_EN			(1 << 1)

#define TBUF_CTRL_V1			0x80
#define TBUF_BP_MC_V1			0xA0

#define HFB_CTRL			0x00
#define HFB_FLT_ENABLE_V3PLUS		0x04
#define HFB_FLT_LEN_V2			0x04
#define HFB_FLT_LEN_V3PLUS		0x1C

// uniMac intrl2 registers
#define INTRL2_CPU_STAT			0x00
#define INTRL2_CPU_SET			0x04
#define INTRL2_CPU_CLEAR		0x08
#define INTRL2_CPU_MASK_STATUS		0x0C
#define INTRL2_CPU_MASK_SET		0x10
#define INTRL2_CPU_MASK_CLEAR		0x14

// INTRL2 instance 0 definitions
#define UMAC_IRQ_SCB			(1 << 0)
#define UMAC_IRQ_EPHY			(1 << 1)
#define UMAC_IRQ_PHY_DET_R		(1 << 2)
#define UMAC_IRQ_PHY_DET_F		(1 << 3)
#define UMAC_IRQ_LINK_UP		(1 << 4)
#define UMAC_IRQ_LINK_DOWN		(1 << 5)
#define UMAC_IRQ_LINK_EVENT		(UMAC_IRQ_LINK_UP | UMAC_IRQ_LINK_DOWN)
#define UMAC_IRQ_UMAC			(1 << 6)
#define UMAC_IRQ_UMAC_TSV		(1 << 7)
#define UMAC_IRQ_TBUF_UNDERRUN		(1 << 8)
#define UMAC_IRQ_RBUF_OVERFLOW		(1 << 9)
#define UMAC_IRQ_HFB_SM			(1 << 10)
#define UMAC_IRQ_HFB_MM			(1 << 11)
#define UMAC_IRQ_MPD_R			(1 << 12)
#define UMAC_IRQ_RXDMA_MBDONE		(1 << 13)
#define UMAC_IRQ_RXDMA_PDONE		(1 << 14)
#define UMAC_IRQ_RXDMA_BDONE		(1 << 15)
#define UMAC_IRQ_RXDMA_DONE		UMAC_IRQ_RXDMA_MBDONE
#define UMAC_IRQ_TXDMA_MBDONE		(1 << 16)
#define UMAC_IRQ_TXDMA_PDONE		(1 << 17)
#define UMAC_IRQ_TXDMA_BDONE		(1 << 18)
#define UMAC_IRQ_TXDMA_DONE		UMAC_IRQ_TXDMA_MBDONE

// Only valid for GENETv3+
#define UMAC_IRQ_MDIO_DONE		(1 << 23)
#define UMAC_IRQ_MDIO_ERROR		(1 << 24)

// INTRL2 instance 1 definitions
#define UMAC_IRQ1_TX_INTR_MASK		0xFFFF
#define UMAC_IRQ1_RX_INTR_MASK		0xFFFF
#define UMAC_IRQ1_RX_INTR_SHIFT		16

// Register block offsets
#define GENET_SYS_OFF			0x0000
#define GENET_GR_BRIDGE_OFF		0x0040
#define GENET_EXT_OFF			0x0080
#define GENET_INTRL2_0_OFF		0x0200
#define GENET_INTRL2_1_OFF		0x0240
#define GENET_RBUF_OFF			0x0300
#define GENET_UMAC_OFF			0x0800

// SYS block offsets and register definitions
#define SYS_REV_CTRL			0x00
#define SYS_PORT_CTRL			0x04
#define  PORT_MODE_INT_EPHY		0
#define  PORT_MODE_INT_GPHY		1
#define  PORT_MODE_EXT_EPHY		2
#define  PORT_MODE_EXT_GPHY		3
#define  PORT_MODE_EXT_RVMII_25		(4 | BIT(4))
#define  PORT_MODE_EXT_RVMII_50		4
#define  LED_ACT_SOURCE_MAC		(1 << 9)

#define SYS_RBUF_FLUSH_CTRL		0x08
#define SYS_TBUF_FLUSH_CTRL		0x0C
#define RBUF_FLUSH_CTRL_V1		0x04

// Ext block register offsets and definitions
#define EXT_EXT_PWR_MGMT		0x00
#define  EXT_PWR_DOWN_BIAS		(1 << 0)
#define  EXT_PWR_DOWN_DLL		(1 << 1)
#define  EXT_PWR_DOWN_PHY		(1 << 2)
#define  EXT_PWR_DN_EN_LD		(1 << 3)
#define  EXT_ENERGY_DET			(1 << 4)
#define  EXT_IDDQ_FROM_PHY		(1 << 5)
#define  EXT_IDDQ_GLBL_PWR		(1 << 7)
#define  EXT_PHY_RESET			(1 << 8)
#define  EXT_ENERGY_DET_MASK		(1 << 12)
#define  EXT_PWR_DOWN_PHY_TX		(1 << 16)
#define  EXT_PWR_DOWN_PHY_RX		(1 << 17)
#define  EXT_PWR_DOWN_PHY_SD		(1 << 18)
#define  EXT_PWR_DOWN_PHY_RD		(1 << 19)
#define  EXT_PWR_DOWN_PHY_EN		(1 << 20)

#define EXT_RGMII_OOB_CTRL		0x0C
#define  RGMII_LINK			(1 << 4)
#define  OOB_DISABLE			(1 << 5)
#define  RGMII_MODE_EN			(1 << 6)
#define  ID_MODE_DIS			(1 << 16)

#define EXT_GPHY_CTRL			0x1C
#define  EXT_CFG_IDDQ_BIAS		(1 << 0)
#define  EXT_CFG_PWR_DOWN		(1 << 1)
#define  EXT_CK25_DIS			(1 << 4)
#define  EXT_GPHY_RESET			(1 << 5)

// DMA rings size
#define DMA_RING_SIZE			(0x40)
#define DMA_RINGS_SIZE			(DMA_RING_SIZE * (GENET_DESC_INDEX + 1))

// DMA registers common definitions
#define DMA_RW_POINTER_MASK		0x1FF
#define DMA_P_INDEX_DISCARD_CNT_MASK	0xFFFF
#define DMA_P_INDEX_DISCARD_CNT_SHIFT	16
#define DMA_BUFFER_DONE_CNT_MASK	0xFFFF
#define DMA_BUFFER_DONE_CNT_SHIFT	16
#define DMA_P_INDEX_MASK		0xFFFF
#define DMA_C_INDEX_MASK		0xFFFF

// DMA ring size register
#define DMA_RING_SIZE_MASK		0xFFFF
#define DMA_RING_SIZE_SHIFT		16
#define DMA_RING_BUFFER_SIZE_MASK	0xFFFF

// DMA interrupt threshold register
#define DMA_INTR_THRESHOLD_MASK		0x01FF

// DMA XON/XOFF register
#define DMA_XON_THREHOLD_MASK		0xFFFF
#define DMA_XOFF_THRESHOLD_MASK		0xFFFF
#define DMA_XOFF_THRESHOLD_SHIFT	16

// DMA flow period register
#define DMA_FLOW_PERIOD_MASK		0xFFFF
#define DMA_MAX_PKT_SIZE_MASK		0xFFFF
#define DMA_MAX_PKT_SIZE_SHIFT		16


// DMA control register
#define DMA_EN				(1 << 0)
#define DMA_RING_BUF_EN_SHIFT		0x01
#define DMA_RING_BUF_EN_MASK		0xFFFF
#define DMA_TSB_SWAP_EN			(1 << 20)

// DMA status register
#define DMA_DISABLED			(1 << 0)
#define DMA_DESC_RAM_INIT_BUSY		(1 << 1)

// DMA SCB burst size register
#define DMA_SCB_BURST_SIZE_MASK		0x1F

// DMA activity vector register
#define DMA_ACTIVITY_VECTOR_MASK	0x1FFFF

// DMA backpressure mask register
#define DMA_BACKPRESSURE_MASK		0x1FFFF
#define DMA_PFC_ENABLE			(1 << 31)

// DMA backpressure status register
#define DMA_BACKPRESSURE_STATUS_MASK	0x1FFFF

// DMA override register
#define DMA_LITTLE_ENDIAN_MODE		(1 << 0)
#define DMA_REGISTER_MODE		(1 << 1)

// DMA timeout register
#define DMA_TIMEOUT_MASK		0xFFFF
#define DMA_TIMEOUT_VAL			5000	// micro seconds

// TDMA rate limiting control register
#define DMA_RATE_LIMIT_EN_MASK		0xFFFF

// TDMA arbitration control register
#define DMA_ARBITER_MODE_MASK		0x03
#define DMA_RING_BUF_PRIORITY_MASK	0x1F
#define DMA_RING_BUF_PRIORITY_SHIFT	5
#define DMA_PRIO_REG_INDEX(q)		((q) / 6)
#define DMA_PRIO_REG_SHIFT(q)		(((q) % 6) * DMA_RING_BUF_PRIORITY_SHIFT)
#define DMA_RATE_ADJ_MASK		0xFF

// Tx/Rx Dma Descriptor common bits
#define DMA_BUFLENGTH_MASK		0x0fff
#define DMA_BUFLENGTH_SHIFT		16
#define DMA_OWN				0x8000
#define DMA_EOP				0x4000
#define DMA_SOP				0x2000
#define DMA_WRAP			0x1000
// Tx specific Dma descriptor bits
#define DMA_TX_UNDERRUN			0x0200
#define DMA_TX_APPEND_CRC		0x0040
#define DMA_TX_OW_CRC			0x0020
#define DMA_TX_DO_CSUM			0x0010
#define DMA_TX_QTAG_SHIFT		7

// Rx Specific Dma descriptor bits
#define DMA_RX_CHK_V3PLUS		0x8000
#define DMA_RX_CHK_V12			0x1000
#define DMA_RX_BRDCAST			0x0040
#define DMA_RX_MULT			0x0020
#define DMA_RX_LG			0x0010
#define DMA_RX_NO			0x0008
#define DMA_RX_RXER			0x0004
#define DMA_RX_CRC_ERROR		0x0002
#define DMA_RX_OV			0x0001
#define DMA_RX_FI_MASK			0x001F
#define DMA_RX_FI_SHIFT			0x0007
#define DMA_DESC_ALLOC_MASK		0x00FF

#define DMA_ARBITER_RR			0x00
#define DMA_ARBITER_WRR			0x01
#define DMA_ARBITER_SP			0x02

// RX/TX DMA register accessors
enum dma_reg
{
	DMA_RING_CFG = 0,
	DMA_CTRL,
	DMA_STATUS,
	DMA_SCB_BURST_SIZE,
	DMA_ARB_CTRL,
	DMA_PRIORITY_0,
	DMA_PRIORITY_1,
	DMA_PRIORITY_2,
	DMA_INDEX2RING_0,
	DMA_INDEX2RING_1,
	DMA_INDEX2RING_2,
	DMA_INDEX2RING_3,
	DMA_INDEX2RING_4,
	DMA_INDEX2RING_5,
	DMA_INDEX2RING_6,
	DMA_INDEX2RING_7,
	DMA_RING0_TIMEOUT,
	DMA_RING1_TIMEOUT,
	DMA_RING2_TIMEOUT,
	DMA_RING3_TIMEOUT,
	DMA_RING4_TIMEOUT,
	DMA_RING5_TIMEOUT,
	DMA_RING6_TIMEOUT,
	DMA_RING7_TIMEOUT,
	DMA_RING8_TIMEOUT,
	DMA_RING9_TIMEOUT,
	DMA_RING10_TIMEOUT,
	DMA_RING11_TIMEOUT,
	DMA_RING12_TIMEOUT,
	DMA_RING13_TIMEOUT,
	DMA_RING14_TIMEOUT,
	DMA_RING15_TIMEOUT,
	DMA_RING16_TIMEOUT,
};

static const u8 dma_regs[] =			// dma_regs_v3plus
{
	0x00, 0x04, 0x08, 0x0C, 0x2C, 0x30, 0x34, 0x38,
	0x70, 0x74, 0x78, 0x7C, 0x80, 0x84, 0x88, 0x8C,
	0x2C, 0x30, 0x34, 0x38, 0x3C, 0x40, 0x44, 0x48,
	0x4C, 0x50, 0x54, 0x58, 0x5C, 0x60, 0x64, 0x68,
	0x6C
};

// RDMA/TDMA ring registers and accessors
// we merge the common fields and just prefix with T/D the registers
// having different meaning depending on the direction
enum dma_ring_reg
{
	TDMA_READ_PTR = 0,
	RDMA_WRITE_PTR = TDMA_READ_PTR,
	TDMA_READ_PTR_HI,
	RDMA_WRITE_PTR_HI = TDMA_READ_PTR_HI,
	TDMA_CONS_INDEX,
	RDMA_PROD_INDEX = TDMA_CONS_INDEX,
	TDMA_PROD_INDEX,
	RDMA_CONS_INDEX = TDMA_PROD_INDEX,
	DMA_RING_BUF_SIZE,
	DMA_START_ADDR,
	DMA_START_ADDR_HI,
	DMA_END_ADDR,
	DMA_END_ADDR_HI,
	DMA_MBUF_DONE_THRESH,
	TDMA_FLOW_PERIOD,
	RDMA_XON_XOFF_THRESH = TDMA_FLOW_PERIOD,
	TDMA_WRITE_PTR,
	RDMA_READ_PTR = TDMA_WRITE_PTR,
	TDMA_WRITE_PTR_HI,
	RDMA_READ_PTR_HI = TDMA_WRITE_PTR_HI
};

// GENET v4+ supports 40-bits pointer addressing
// for obvious reasons the LO and HI word parts
// are contiguous, but this offsets the other
// registers
static const u8 genet_dma_ring_regs[] =		// genet_dma_ring_regs_v4
{
	0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C,
	0x20, 0x24, 0x28, 0x2C, 0x30
};

// Generate I/O inline functions
#define GENET_IO_MACRO(name, offset)				\
static inline u32 name##_readl(u32 off)				\
{								\
	return read32 (ARM_BCM54213_BASE + offset + off);	\
}								\
static inline void name##_writel(u32 val, u32 off)		\
{								\
	write32 (ARM_BCM54213_BASE + offset + off, val);	\
}

GENET_IO_MACRO(ext, GENET_EXT_OFF);
GENET_IO_MACRO(umac, GENET_UMAC_OFF);
GENET_IO_MACRO(sys, GENET_SYS_OFF);

// interrupt l2 registers accessors
GENET_IO_MACRO(intrl2_0, GENET_INTRL2_0_OFF);
GENET_IO_MACRO(intrl2_1, GENET_INTRL2_1_OFF);

// HFB register accessors
GENET_IO_MACRO(hfb, HFB_OFFSET);

// GENET v2+ HFB control and filter len helpers
GENET_IO_MACRO(hfb_reg, HFB_REG_OFFSET);

// RBUF register accessors
GENET_IO_MACRO(rbuf, GENET_RBUF_OFF);

// more I/O helper macros
#define rbuf_ctrl_get()			sys_readl(SYS_RBUF_FLUSH_CTRL)
#define rbuf_ctrl_set(val)		sys_writel(val, SYS_RBUF_FLUSH_CTRL)

#define tdma_readl(r)			read32(ARM_BCM54213_BASE + GENET_TDMA_REG_OFF +  \
					       DMA_RINGS_SIZE + dma_regs[r])
#define tdma_writel(val, r)		write32(ARM_BCM54213_BASE + GENET_TDMA_REG_OFF + \
						DMA_RINGS_SIZE + dma_regs[r], val)

#define rdma_readl(r)			read32(ARM_BCM54213_BASE + GENET_RDMA_REG_OFF +  \
					       DMA_RINGS_SIZE + dma_regs[r])
#define rdma_writel(val, r)		write32(ARM_BCM54213_BASE + GENET_RDMA_REG_OFF + \
						DMA_RINGS_SIZE + dma_regs[r], val)

#define tdma_ring_readl(ring, r)	read32(ARM_BCM54213_BASE + GENET_TDMA_REG_OFF +  \
						       (DMA_RING_SIZE * ring) + 	 \
						       genet_dma_ring_regs[r])
#define tdma_ring_writel(ring, val, r)	write32(ARM_BCM54213_BASE + GENET_TDMA_REG_OFF + \
							(DMA_RING_SIZE * ring) +	 \
							genet_dma_ring_regs[r], val)

#define rdma_ring_readl(ring, r)	read32(ARM_BCM54213_BASE + GENET_RDMA_REG_OFF +  \
						       (DMA_RING_SIZE * ring) +		 \
							genet_dma_ring_regs[r])
#define rdma_ring_writel(ring, val, r)	write32(ARM_BCM54213_BASE + GENET_RDMA_REG_OFF + \
							(DMA_RING_SIZE * ring) +	 \
							genet_dma_ring_regs[r], val)

#define dmadesc_get_length_status(d)	read32(d + DMA_DESC_LENGTH_STATUS)

static const char FromBcm54213[] = "genet";

CBcm54213Device::CBcm54213Device (void)
:	m_pTimer (CTimer::Get ()),
	m_bInterruptConnected (FALSE),
	m_tx_cbs (0),
	m_rx_cbs (0)
{
	assert (m_pTimer != 0);
}

CBcm54213Device::~CBcm54213Device (void)
{
	intr_disable ();

	dma_disable ();
	init_tx_queues (false);

	umac_reset2 ();
	reset_umac ();

	if (m_bInterruptConnected)
	{
		CInterruptSystem::Get ()->DisconnectIRQ (ARM_IRQ_BCM54213_0);
		CInterruptSystem::Get ()->DisconnectIRQ (ARM_IRQ_BCM54213_1);
	}

	if (m_rx_cbs != 0)
	{
		free_rx_buffers ();
	}

	delete [] m_tx_cbs;
	delete [] m_rx_cbs;
}

boolean CBcm54213Device::Initialize (void)
{
	// probe:

	u32 reg = sys_readl (SYS_REV_CTRL);	// read GENET HW version
	u8 major = (reg >> 24 & 0x0f);
	if (major == 6)
		major = 5;
	else if (major == 5)
		major = 4;
	else if (major == 0)
		major = 1;
	if (major != GENET_V5)
	{
		CLogger::Get ()->Write (FromBcm54213, LogError,
					"GENET version mismatch, got: %d, configured for: %d",
					(unsigned) major, GENET_V5);

		return FALSE;
	}

	assert (((reg >> 16) & 0x0F) == 0);	// minor version
	assert ((reg & 0xFFFF) == 0);		// EPHY version

	reset_umac ();

	// open:

	umac_reset2 ();
	init_umac ();

	reg = umac_readl(UMAC_CMD);		// make sure we reflect the value of CRC_CMD_FWD
	m_crc_fwd_en = !!(reg & CMD_CRC_FWD);

	int ret = set_hw_addr();
	if (ret)
	{
		CLogger::Get ()->Write (FromBcm54213, LogError, "Cannot set MAC address (%d)", ret);

		return FALSE;
	}

	u32 dma_ctrl = dma_disable();		// disable Rx/Tx DMA and flush Tx queues

	ret = init_dma();			// reinitialize TDMA and RDMA and SW housekeeping
	if (ret)
	{
		CLogger::Get ()->Write (FromBcm54213, LogError, "Failed to initialize DMA (%d)", ret);

		return FALSE;
	}

	enable_dma(dma_ctrl);			// always enable ring 16 - descriptor ring

	hfb_init();

	assert (!m_bInterruptConnected);
	CInterruptSystem::Get ()->ConnectIRQ (ARM_IRQ_BCM54213_0, InterruptStub0, this);
	CInterruptSystem::Get ()->ConnectIRQ (ARM_IRQ_BCM54213_1, InterruptStub1, this);
	m_bInterruptConnected = TRUE;

	ret = mii_probe();
	if (ret)
	{
		CLogger::Get ()->Write (FromBcm54213, LogError, "Failed to connect to PHY (%d)", ret);

		CInterruptSystem::Get ()->DisconnectIRQ (ARM_IRQ_BCM54213_0);
		CInterruptSystem::Get ()->DisconnectIRQ (ARM_IRQ_BCM54213_1);
		m_bInterruptConnected = FALSE;

		return FALSE;
	}

	netif_start();

	set_rx_mode ();

	AddNetDevice ();

	return TRUE;
}

const CMACAddress *CBcm54213Device::GetMACAddress (void) const
{
	return &m_MACAddress;
}

boolean CBcm54213Device::IsSendFrameAdvisable (void)
{
	unsigned index = TX_RING_INDEX;			// see SendFrame() for mapping strategy
	if (index == 0)
		index = GENET_DESC_INDEX;
	else
		index -= 1;
							// is there room for a frame?
	return m_tx_rings[index].free_bds >= 2;		// atomic read
}

boolean CBcm54213Device::SendFrame (const void *pBuffer, unsigned nLength)
{
	assert (pBuffer != 0);
	assert (nLength > 0);

	// Mapping strategy:
	// index = 0, unclassified, packet xmited through ring16
	// index = 1, goes to ring 0. (highest priority queue)
	// index = 2, goes to ring 1.
	// index = 3, goes to ring 2.
	// index = 4, goes to ring 3.
	unsigned index = TX_RING_INDEX;
	if (index == 0)
		index = GENET_DESC_INDEX;
	else
		index -= 1;

	TGEnetTxRing *ring = &m_tx_rings[index];

	m_TxSpinLock.Acquire ();

	if (ring->free_bds < 2)				// is there room for this frame?
	{
		CLogger::Get ()->Write (FromBcm54213, LogWarning, "TX frame dropped");

		m_TxSpinLock.Release ();

		return FALSE;
	}

	u8 *pTxBuffer = new u8[ENET_MAX_MTU_SIZE];	// allocate and fill DMA buffer
	memcpy (pTxBuffer, pBuffer, nLength);
	if (nLength < ETH_ZLEN)				// pad frame if necessary
	{
		memset (pTxBuffer+nLength, 0, ETH_ZLEN-nLength);
		nLength = ETH_ZLEN;
	}

	TGEnetCB *tx_cb_ptr = get_txcb (ring);		// get Tx control block from ring
	assert (tx_cb_ptr != 0);

	// prepare for DMA
	CleanAndInvalidateDataCacheRange ((u32) (uintptr) pTxBuffer, nLength);

	tx_cb_ptr->buffer = pTxBuffer;			// set DMA buffer in Tx control block

	// set DMA descriptor and start transfer
	dmadesc_set (tx_cb_ptr->bd_addr, pTxBuffer,   (nLength << DMA_BUFLENGTH_SHIFT)
						    | (QTAG_MASK << DMA_TX_QTAG_SHIFT)
						    | DMA_TX_APPEND_CRC | DMA_SOP | DMA_EOP);

	// decrement total BD count and advance our write pointer
	ring->free_bds--;
	ring->prod_index++;
	ring->prod_index &= DMA_P_INDEX_MASK;

	// packets are ready, update producer index
	tdma_ring_writel(ring->index, ring->prod_index, TDMA_PROD_INDEX);

	m_TxSpinLock.Release ();

	return TRUE;
}

boolean CBcm54213Device::ReceiveFrame (void *pBuffer, unsigned *pResultLength)
{
	assert (pBuffer != 0);
	assert (pResultLength != 0);

	TGEnetRxRing *ring = &m_rx_rings[GENET_DESC_INDEX];	// the only supported Rx queue

	// clear status before servicing to reduce spurious interrupts
	// NOTE: Rx interrupts are not used
	//intrl2_0_writel (UMAC_IRQ_RXDMA_DONE, INTRL2_CPU_CLEAR);

	unsigned p_index = rdma_ring_readl (ring->index, RDMA_PROD_INDEX);

	unsigned discards =   (p_index >> DMA_P_INDEX_DISCARD_CNT_SHIFT)
			    & DMA_P_INDEX_DISCARD_CNT_MASK;
	if (discards > ring->old_discards)
	{
		discards = discards - ring->old_discards;
		ring->old_discards += discards;

		// clear HW register when we reach 75% of maximum 0xFFFF
		if (ring->old_discards >= 0xC000)
		{
			ring->old_discards = 0;
			rdma_ring_writel (ring->index, 0, RDMA_PROD_INDEX);
		}
	}

	p_index &= DMA_P_INDEX_MASK;

	boolean bResult = FALSE;

	unsigned rxpkttoprocess = (p_index - ring->c_index) & DMA_C_INDEX_MASK;
	if (rxpkttoprocess > 0)
	{
		u32 dma_length_status;
		u32 dma_flag;
		int nLength;

		TGEnetCB *cb = &m_rx_cbs[ring->read_ptr];

		u8 *pRxBuffer = rx_refill (cb);
		if (pRxBuffer == 0)
		{
			CLogger::Get ()->Write (FromBcm54213, LogWarning, "Missing RX buffer!");

			goto out;
		}

		dma_length_status = dmadesc_get_length_status (cb->bd_addr);
		dma_flag = dma_length_status & 0xFFFF;
		nLength = dma_length_status >> DMA_BUFLENGTH_SHIFT;

		if (   !(dma_flag & DMA_EOP)
		    || !(dma_flag & DMA_SOP))
		{
			CLogger::Get ()->Write (FromBcm54213, LogWarning,
						"Dropping fragmented RX packet!");

			delete [] pRxBuffer;

			goto out;
		}

		// report errors
		if (dma_flag & (DMA_RX_CRC_ERROR | DMA_RX_OV | DMA_RX_NO | DMA_RX_LG | DMA_RX_RXER))
		{
			CLogger::Get ()->Write (FromBcm54213, LogWarning, "RX error (0x%x)",
						(unsigned) dma_flag);

			delete [] pRxBuffer;

			goto out;
		}

#define LEADING_PAD	2
		nLength -= LEADING_PAD;		// remove HW 2 bytes added for IP alignment

		if (m_crc_fwd_en)
		{
			nLength -= ETH_FCS_LEN;
		}


		assert (nLength > 0);
		assert (nLength <= FRAME_BUFFER_SIZE);
		memcpy (pBuffer, pRxBuffer+LEADING_PAD, nLength);

		*pResultLength = nLength;

		delete [] pRxBuffer;

		bResult = TRUE;

out:
		if (ring->read_ptr < ring->end_ptr)
		{
			ring->read_ptr++;
		}
		else
		{
			ring->read_ptr = ring->cb_ptr;
		}

		ring->c_index = (ring->c_index + 1) & DMA_C_INDEX_MASK;
		rdma_ring_writel (ring->index, ring->c_index, RDMA_CONS_INDEX);
	}

	return bResult;
}

boolean CBcm54213Device::IsLinkUp (void)
{
	return m_link ? TRUE : FALSE;
}

TNetDeviceSpeed CBcm54213Device::GetLinkSpeed (void)
{
	if (!m_link)
	{
		return NetDeviceSpeedUnknown;
	}

	switch (m_speed)
	{
	case 10:
		return m_duplex ? NetDeviceSpeed10Full : NetDeviceSpeed10Half;

	case 100:
		return m_duplex ? NetDeviceSpeed100Full : NetDeviceSpeed100Half;

	case 1000:
		return m_duplex ? NetDeviceSpeed1000Full : NetDeviceSpeed1000Half;

	default:
		return NetDeviceSpeedUnknown;
	}
}

boolean CBcm54213Device::UpdatePHY (void)
{
	if (phy_read_status () == 0)
	{
		mii_setup ();
	}

	return TRUE;
}

void CBcm54213Device::reset_umac(void)
{
	// 7358a0/7552a0: bad default in RBUF_FLUSH_CTRL.umac_sw_rst
	rbuf_ctrl_set(0);
	udelay(10);

	// disable MAC while updating its registers
	umac_writel(0, UMAC_CMD);

	// issue soft reset with (rg)mii loopback to ensure a stable rxclk
	umac_writel(CMD_SW_RESET | CMD_LCL_LOOP_EN, UMAC_CMD);
	udelay(2);
	umac_writel(0, UMAC_CMD);
}

void CBcm54213Device::umac_reset2(void)
{
	u32 reg = rbuf_ctrl_get();
	reg |= BIT(1);
	rbuf_ctrl_set(reg);
	udelay(10);

	reg &= ~BIT(1);
	rbuf_ctrl_set(reg);
	udelay(10);
}

void CBcm54213Device::init_umac(void)
{
	reset_umac();

	// clear tx/rx counter
	umac_writel(MIB_RESET_RX | MIB_RESET_TX | MIB_RESET_RUNT, UMAC_MIB_CTRL);
	umac_writel(0, UMAC_MIB_CTRL);

	umac_writel(ENET_MAX_MTU_SIZE, UMAC_MAX_FRAME_LEN);

	// init rx registers, enable ip header optimization
	u32 reg = rbuf_readl(RBUF_CTRL);
	reg |= RBUF_ALIGN_2B;
	rbuf_writel(reg, RBUF_CTRL);

	rbuf_writel(1, RBUF_TBUF_SIZE_CTRL);

	intr_disable();

	// Enable MDIO interrupts on GENET v3+
	// NOTE: MDIO interrupts do not work
	//intrl2_0_writel(UMAC_IRQ_MDIO_DONE | UMAC_IRQ_MDIO_ERROR, INTRL2_CPU_MASK_CLEAR);
}

void CBcm54213Device::umac_enable_set(u32 mask, bool enable)
{
	u32 reg = umac_readl(UMAC_CMD);
	if (enable)
		reg |= mask;

	else
		reg &= ~mask;
	umac_writel(reg, UMAC_CMD);

	// UniMAC stops on a packet boundary, wait for a full-size packet to be processed
	if (enable == 0)
		udelay(2000);
}

void CBcm54213Device::intr_disable(void)
{
	// Mask all interrupts
	intrl2_0_writel(0xFFFFFFFF, INTRL2_CPU_MASK_SET);
	intrl2_0_writel(0xFFFFFFFF, INTRL2_CPU_CLEAR);
	intrl2_1_writel(0xFFFFFFFF, INTRL2_CPU_MASK_SET);
	intrl2_1_writel(0xFFFFFFFF, INTRL2_CPU_CLEAR);
}

void CBcm54213Device::enable_tx_intr(void)
{
	TGEnetTxRing *ring;
	for (unsigned i = 0; i < TX_QUEUES; ++i)
	{
		ring = &m_tx_rings[i];
		ring->int_enable(ring);
	}

	ring = &m_tx_rings[GENET_DESC_INDEX];
	ring->int_enable(ring);
}

void CBcm54213Device::enable_rx_intr(void)
{
	TGEnetRxRing *ring = &m_rx_rings[GENET_DESC_INDEX];
	ring->int_enable(ring);
}

void CBcm54213Device::link_intr_enable(void)
{
	intrl2_0_writel(UMAC_IRQ_LINK_EVENT, INTRL2_CPU_MASK_CLEAR);
}

void CBcm54213Device::tx_ring16_int_enable(TGEnetTxRing *ring)
{
	intrl2_0_writel(UMAC_IRQ_TXDMA_DONE, INTRL2_CPU_MASK_CLEAR);
}

void CBcm54213Device::tx_ring_int_enable(TGEnetTxRing *ring)
{
	intrl2_1_writel(1 << ring->index, INTRL2_CPU_MASK_CLEAR);
}

void CBcm54213Device::rx_ring16_int_enable(TGEnetRxRing *ring)
{
	intrl2_0_writel(UMAC_IRQ_RXDMA_DONE, INTRL2_CPU_MASK_CLEAR);
}

int CBcm54213Device::set_hw_addr(void)
{
	CBcmPropertyTags Tags;
	TPropertyTagMACAddress MACAddress;
	if (!Tags.GetTag (PROPTAG_GET_MAC_ADDRESS, &MACAddress, sizeof MACAddress))
	{
		return -1;
	}

	m_MACAddress.Set (MACAddress.Address);

	CString MACString;
	m_MACAddress.Format (&MACString);
	CLogger::Get ()->Write (FromBcm54213, LogDebug, "MAC address is %s",
				(const char *) MACString);

	umac_writel(  (MACAddress.Address[0] << 24)
		    | (MACAddress.Address[1] << 16)
		    | (MACAddress.Address[2] << 8)
		    |  MACAddress.Address[3], UMAC_MAC0);
	umac_writel((  MACAddress.Address[4] << 8)
		     | MACAddress.Address[5], UMAC_MAC1);

	return 0;
}

#define MAX_MC_COUNT	16

void CBcm54213Device::set_mdf_addr(unsigned char *addr, int *i, int *mc)
{
	umac_writel(  addr[0] << 8
		    | addr[1], UMAC_MDF_ADDR + (*i * 4));
	umac_writel(  addr[2] << 24
		    | addr[3] << 16
		    | addr[4] << 8
		    | addr[5], UMAC_MDF_ADDR + ((*i + 1) * 4));

	u32 reg = umac_readl(UMAC_MDF_CTRL);
	reg |= (1 << (MAX_MC_COUNT - *mc));
	umac_writel(reg, UMAC_MDF_CTRL);

	*i += 2;
	(*mc)++;
}

void CBcm54213Device::set_rx_mode(void)
{
	// Promiscuous mode off
	u32 reg = umac_readl(UMAC_CMD);
	reg &= ~CMD_PROMISC;
	umac_writel(reg, UMAC_CMD);

	// update MDF filter
	int i = 0;
	int mc = 0;

	u8 Buffer[MAC_ADDRESS_SIZE];

	// Broadcast
	CMACAddress Broadcast;
	Broadcast.SetBroadcast();
	Broadcast.CopyTo(Buffer);
	set_mdf_addr(Buffer, &i, &mc);

	// my own address
	m_MACAddress.CopyTo(Buffer);
	set_mdf_addr(Buffer, &i, &mc);
}

// clear Hardware Filter Block and disable all filtering
void CBcm54213Device::hfb_init(void)
{
	// this has no function, but to suppress warnings from clang compiler >>>
	hfb_reg_readl (HFB_CTRL);
	hfb_readl (0);
	// <<<

	hfb_reg_writel(0, HFB_CTRL);
	hfb_reg_writel(0, HFB_FLT_ENABLE_V3PLUS);
	hfb_reg_writel(0, HFB_FLT_ENABLE_V3PLUS + 4);

	u32 i;
	for (i = DMA_INDEX2RING_0; i <= DMA_INDEX2RING_7; i++)
		rdma_writel(0, i);

	for (i = 0; i < (HFB_FILTER_CNT / 4); i++)
		hfb_reg_writel(0, HFB_FLT_LEN_V3PLUS + i * sizeof(u32));

	for (i = 0; i < HFB_FILTER_CNT * HFB_FILTER_SIZE; i++)
		hfb_writel(0, i * sizeof(u32));
}

// Start the network engine
void CBcm54213Device::netif_start(void)
{
	//enable_rx_intr();		// NOTE: Rx interrupts are not needed

	umac_enable_set(CMD_TX_EN | CMD_RX_EN, true);

	enable_tx_intr();

	//link_intr_enable();		// NOTE: link interrupts do not work, must be polled
}

// Initialize DMA control register
int CBcm54213Device::init_dma(void)
{
	// Initialize common Rx ring structures
	m_rx_cbs = new TGEnetCB[TOTAL_DESC];
	if (!m_rx_cbs)
		return -1;

	memset (m_rx_cbs, 0, TOTAL_DESC * sizeof (TGEnetCB));

	TGEnetCB *cb;
	unsigned i;
	for (i = 0; i < TOTAL_DESC; i++)
	{
		cb = m_rx_cbs + i;
		cb->bd_addr = ARM_BCM54213_BASE + RDMA_OFFSET + i * DMA_DESC_SIZE;
	}

	// Initialize common TX ring structures
	m_tx_cbs = new TGEnetCB[TOTAL_DESC];
	if (!m_tx_cbs)
	{
		delete [] m_rx_cbs;
		m_rx_cbs = 0;
		return -1;
	}

	memset (m_tx_cbs, 0, TOTAL_DESC * sizeof (TGEnetCB));

	for (i = 0; i < TOTAL_DESC; i++)
	{
		cb = m_tx_cbs + i;
		cb->bd_addr = ARM_BCM54213_BASE + TDMA_OFFSET + i * DMA_DESC_SIZE;
	}

	// Init rDma
	rdma_writel(DMA_MAX_BURST_LENGTH, DMA_SCB_BURST_SIZE);

	// Initialize Rx queues
	int ret = init_rx_queues();
	if (ret)
	{
		CLogger::Get ()->Write (FromBcm54213, LogError,
					"Failed to initialize RX queues (%d)", ret);
		free_rx_buffers();
		delete [] m_rx_cbs;
		delete [] m_tx_cbs;
		return ret;
	}

	// Init tDma
	tdma_writel(DMA_MAX_BURST_LENGTH, DMA_SCB_BURST_SIZE);

	// Initialize Tx queues
	init_tx_queues(true);

	return 0;
}

// Returns a reusable dma control register value
u32 CBcm54213Device::dma_disable(void)
{
	// disable DMA
	u32 dma_ctrl = 1 << (GENET_DESC_INDEX + DMA_RING_BUF_EN_SHIFT) | DMA_EN;
	u32 reg = tdma_readl(DMA_CTRL);
	reg &= ~dma_ctrl;
	tdma_writel(reg, DMA_CTRL);

	reg = rdma_readl(DMA_CTRL);
	reg &= ~dma_ctrl;
	rdma_writel(reg, DMA_CTRL);

	umac_writel(1, UMAC_TX_FLUSH);
	udelay(10);
	umac_writel(0, UMAC_TX_FLUSH);

	return dma_ctrl;
}

void CBcm54213Device::enable_dma(u32 dma_ctrl)
{
	u32 reg = rdma_readl(DMA_CTRL);
	reg |= dma_ctrl;
	rdma_writel(reg, DMA_CTRL);

	reg = tdma_readl(DMA_CTRL);
	reg |= dma_ctrl;
	tdma_writel(reg, DMA_CTRL);
}

// Initialize or reset Tx queues
//
// Queues 0-3 are priority-based, each one has 32 descriptors,
// with queue 0 being the highest priority queue.
//
// Queue 16 is the default Tx queue with
// GENET_Q16_TX_BD_CNT = 256 - 4 * 32 = 128 descriptors.
//
// The transmit control block pool is then partitioned as follows:
// - Tx queue 0 uses m_tx_cbs[0..31]
// - Tx queue 1 uses m_tx_cbs[32..63]
// - Tx queue 2 uses m_tx_cbs[64..95]
// - Tx queue 3 uses m_tx_cbs[96..127]
// - Tx queue 16 uses m_tx_cbs[128..255]
void CBcm54213Device::init_tx_queues(bool enable)
{
	u32 dma_ctrl = tdma_readl(DMA_CTRL);
	u32 dma_enable = dma_ctrl & DMA_EN;
	dma_ctrl &= ~DMA_EN;
	tdma_writel(dma_ctrl, DMA_CTRL);

	dma_ctrl = 0;
	u32 ring_cfg = 0;

	if (enable)
	{
		// Enable strict priority arbiter mode
		tdma_writel(DMA_ARBITER_SP, DMA_ARB_CTRL);
	}

	u32 dma_priority[3] = {0, 0, 0};

	// Initialize Tx priority queues
	for (unsigned i = 0; i < TX_QUEUES; i++) {
		init_tx_ring(i, TX_BDS_PER_Q, i * TX_BDS_PER_Q, (i + 1) * TX_BDS_PER_Q);
		ring_cfg |= (1 << i);
		dma_ctrl |= (1 << (i + DMA_RING_BUF_EN_SHIFT));
		dma_priority[DMA_PRIO_REG_INDEX(i)] |=
			((GENET_Q0_PRIORITY + i) << DMA_PRIO_REG_SHIFT(i));
	}

	// Initialize Tx default queue 16
	init_tx_ring(GENET_DESC_INDEX, GENET_Q16_TX_BD_CNT, TX_QUEUES * TX_BDS_PER_Q, TOTAL_DESC);
	ring_cfg |= (1 << GENET_DESC_INDEX);
	dma_ctrl |= (1 << (GENET_DESC_INDEX + DMA_RING_BUF_EN_SHIFT));
	dma_priority[DMA_PRIO_REG_INDEX(GENET_DESC_INDEX)] |=
		((GENET_Q0_PRIORITY + TX_QUEUES) << DMA_PRIO_REG_SHIFT(GENET_DESC_INDEX));

	if (enable)
	{
		// Set Tx queue priorities
		tdma_writel(dma_priority[0], DMA_PRIORITY_0);
		tdma_writel(dma_priority[1], DMA_PRIORITY_1);
		tdma_writel(dma_priority[2], DMA_PRIORITY_2);

		// Enable Tx queues
		tdma_writel(ring_cfg, DMA_RING_CFG);

		// Enable Tx DMA
		if (dma_enable)
			dma_ctrl |= DMA_EN;
		tdma_writel(dma_ctrl, DMA_CTRL);
	}
	else
	{
		// Disable Tx queues
		tdma_writel(0, DMA_RING_CFG);

		// Disable Tx DMA
		tdma_writel(0, DMA_CTRL);
	}
}

// Initialize a Tx ring along with corresponding hardware registers
void CBcm54213Device::init_tx_ring(unsigned index, unsigned size,
				   unsigned start_ptr, unsigned end_ptr)
{
	TGEnetTxRing *ring = &m_tx_rings[index];

	ring->index = index;
	if (index == GENET_DESC_INDEX) {
		ring->queue = 0;
		ring->int_enable = tx_ring16_int_enable;
	} else {
		ring->queue = index + 1;
		ring->int_enable = tx_ring_int_enable;
	}
	ring->cbs = m_tx_cbs + start_ptr;
	ring->size = size;
	ring->clean_ptr = start_ptr;
	ring->c_index = 0;
	ring->free_bds = size;
	ring->write_ptr = start_ptr;
	ring->cb_ptr = start_ptr;
	ring->end_ptr = end_ptr - 1;
	ring->prod_index = 0;

	// Set flow period for ring != 16
	u32 flow_period_val = 0;
	if (index != GENET_DESC_INDEX)
		flow_period_val = ENET_MAX_MTU_SIZE << 16;

	tdma_ring_writel(index, 0, TDMA_PROD_INDEX);
	tdma_ring_writel(index, 0, TDMA_CONS_INDEX);
	tdma_ring_writel(index, 10, DMA_MBUF_DONE_THRESH);
	// Disable rate control for now
	tdma_ring_writel(index, flow_period_val, TDMA_FLOW_PERIOD);
	tdma_ring_writel(index, ((size << DMA_RING_SIZE_SHIFT) | RX_BUF_LENGTH), DMA_RING_BUF_SIZE);

	// Set start and end address, read and write pointers
	tdma_ring_writel(index, start_ptr * WORDS_PER_BD, DMA_START_ADDR);
	tdma_ring_writel(index, start_ptr * WORDS_PER_BD, TDMA_READ_PTR);
	tdma_ring_writel(index, start_ptr * WORDS_PER_BD, TDMA_WRITE_PTR);
	tdma_ring_writel(index, end_ptr * WORDS_PER_BD-1, DMA_END_ADDR);
}

struct TGEnetCB *CBcm54213Device::get_txcb(TGEnetTxRing *ring)
{
	TGEnetCB *tx_cb_ptr = ring->cbs;
	tx_cb_ptr += ring->write_ptr - ring->cb_ptr;

	// Advancing local write pointer
	if (ring->write_ptr == ring->end_ptr)
		ring->write_ptr = ring->cb_ptr;
	else
		ring->write_ptr++;

	return tx_cb_ptr;
}

unsigned CBcm54213Device::tx_reclaim(TGEnetTxRing *ring)
{
	// Clear status before servicing to reduce spurious interrupts
	if (ring->index == GENET_DESC_INDEX)
		intrl2_0_writel(UMAC_IRQ_TXDMA_DONE, INTRL2_CPU_CLEAR);
	else
		intrl2_1_writel((1 << ring->index), INTRL2_CPU_CLEAR);

	// Compute how many buffers are transmitted since last xmit call
	unsigned c_index = tdma_ring_readl(ring->index, TDMA_CONS_INDEX) & DMA_C_INDEX_MASK;
	unsigned txbds_ready = (c_index - ring->c_index) & DMA_C_INDEX_MASK;

	// Reclaim transmitted buffers
	unsigned txbds_processed = 0;
	while (txbds_processed < txbds_ready) {
		free_tx_cb(&m_tx_cbs[ring->clean_ptr]);
		txbds_processed++;
		if (ring->clean_ptr < ring->end_ptr)
			ring->clean_ptr++;
		else
			ring->clean_ptr = ring->cb_ptr;
	}

	ring->free_bds += txbds_processed;
	ring->c_index = c_index;

	return txbds_processed;
}

void CBcm54213Device::free_tx_cb(TGEnetCB *cb)
{
	u8 *buffer = cb->buffer;
	if (buffer) {
		cb->buffer = 0;

		delete [] buffer;
	}
}

// Initialize Rx queue
// Queue 0-15 are not available
// Queue 16 is the default Rx queue with GENET_Q16_RX_BD_CNT descriptors.
int CBcm54213Device::init_rx_queues(void)
{
	u32 dma_ctrl = rdma_readl(DMA_CTRL);
	u32 dma_enable = dma_ctrl & DMA_EN;
	dma_ctrl &= ~DMA_EN;
	rdma_writel(dma_ctrl, DMA_CTRL);

	dma_ctrl = 0;
	u32 ring_cfg = 0;

	// Initialize Rx default queue 16
	int ret = init_rx_ring(GENET_DESC_INDEX, GENET_Q16_RX_BD_CNT,
			       RX_QUEUES * RX_BDS_PER_Q, TOTAL_DESC);
	if (ret)
		return ret;

	ring_cfg |= (1 << GENET_DESC_INDEX);
	dma_ctrl |= (1 << (GENET_DESC_INDEX + DMA_RING_BUF_EN_SHIFT));

	// Enable rings
	rdma_writel(ring_cfg, DMA_RING_CFG);

	// Configure ring as descriptor ring and re-enable DMA if enabled
	if (dma_enable)
		dma_ctrl |= DMA_EN;
	rdma_writel(dma_ctrl, DMA_CTRL);

	return 0;
}

// Initialize a RDMA ring
int CBcm54213Device::init_rx_ring(unsigned index, unsigned size,
				  unsigned start_ptr, unsigned end_ptr)
{
	TGEnetRxRing *ring = &m_rx_rings[index];

	ring->index = index;

	assert (index == GENET_DESC_INDEX);
	ring->int_enable = rx_ring16_int_enable;

	ring->cbs = m_rx_cbs + start_ptr;
	ring->size = size;
	ring->c_index = 0;
	ring->read_ptr = start_ptr;
	ring->cb_ptr = start_ptr;
	ring->end_ptr = end_ptr - 1;
	ring->old_discards = 0;

	int ret = alloc_rx_buffers(ring);
	if (ret)
		return ret;

	rdma_ring_writel(index, 0, RDMA_PROD_INDEX);
	rdma_ring_writel(index, 0, RDMA_CONS_INDEX);
	rdma_ring_writel(index, ((size << DMA_RING_SIZE_SHIFT) | RX_BUF_LENGTH), DMA_RING_BUF_SIZE);
	rdma_ring_writel(index,   (DMA_FC_THRESH_LO << DMA_XOFF_THRESHOLD_SHIFT)
				|  DMA_FC_THRESH_HI, RDMA_XON_XOFF_THRESH);

	// Set start and end address, read and write pointers
	rdma_ring_writel(index, start_ptr * WORDS_PER_BD, DMA_START_ADDR);
	rdma_ring_writel(index, start_ptr * WORDS_PER_BD, RDMA_READ_PTR);
	rdma_ring_writel(index, start_ptr * WORDS_PER_BD, RDMA_WRITE_PTR);
	rdma_ring_writel(index, end_ptr * WORDS_PER_BD-1, DMA_END_ADDR);

	return ret;
}

// Assign DMA buffer to Rx DMA descriptor
int CBcm54213Device::alloc_rx_buffers(TGEnetRxRing *ring)
{
	// loop here for each buffer needing assign
	for (unsigned i = 0; i < ring->size; i++) {
		TGEnetCB *cb = ring->cbs + i;
		rx_refill(cb);
		if (!cb->buffer)
			return -1;
	}

	return 0;
}

void CBcm54213Device::free_rx_buffers(void)
{
	for (unsigned i = 0; i < TOTAL_DESC; i++)
	{
		TGEnetCB *cb = &m_rx_cbs[i];
		u8 *buffer = free_rx_cb(cb);
		delete [] buffer;
	}
}

u8 *CBcm54213Device::rx_refill(struct TGEnetCB *cb)
{
	// Allocate a new Rx DMA buffer
	u8 *buffer = new u8[RX_BUF_LENGTH];
	if (!buffer)
		return 0;

	// prepare buffer for DMA
	CleanAndInvalidateDataCacheRange ((u32) (uintptr) buffer, RX_BUF_LENGTH);

	// Grab the current Rx buffer from the ring and DMA-unmap it
	u8 *rx_buffer = free_rx_cb(cb);

	// Put the new Rx buffer on the ring
	cb->buffer = buffer;
	dmadesc_set_addr(cb->bd_addr, buffer);

	// Return the current Rx buffer to caller
	return rx_buffer;
}

u8 *CBcm54213Device::free_rx_cb(TGEnetCB *cb)
{
	u8 *buffer = cb->buffer;
	cb->buffer = 0;

	CleanAndInvalidateDataCacheRange ((u32) (uintptr) buffer, RX_BUF_LENGTH);

	return buffer;
}

// Combined address + length/status setter
void CBcm54213Device::dmadesc_set(uintptr d, u8 *addr, u32 value)
{
	dmadesc_set_addr(d, addr);
	dmadesc_set_length_status(d, value);
}

void CBcm54213Device::dmadesc_set_addr(uintptr d, u8 *addr)
{
	// should use BUS_ADDRESS() here, but does not work
	write32(d + DMA_DESC_ADDRESS_LO, (u32) (uintptr) addr);

	// Register writes to GISB bus can take couple hundred nanoseconds
	// and are done for each packet, save these expensive writes unless
	// the platform is explicitly configured for 64-bits/LPAE.
	// TODO: write DMA_DESC_ADDRESS_HI only once
	write32(d + DMA_DESC_ADDRESS_HI, 0);
}

void CBcm54213Device::dmadesc_set_length_status(uintptr d, u32 value)
{
	write32(d + DMA_DESC_LENGTH_STATUS, value);
}

void CBcm54213Device::udelay (unsigned nMicroSeconds)
{
	m_pTimer->usDelay (nMicroSeconds);
}

// handle Rx and Tx default queues + other stuff
void CBcm54213Device::InterruptHandler0 (void)
{
	// Read irq status
	unsigned status =  intrl2_0_readl(INTRL2_CPU_STAT) &
			  ~intrl2_0_readl(INTRL2_CPU_MASK_STATUS);

	// clear interrupts
	intrl2_0_writel(status, INTRL2_CPU_CLEAR);

	if (status & UMAC_IRQ_TXDMA_DONE) {
		m_TxSpinLock.Acquire ();

		TGEnetTxRing *tx_ring = &m_tx_rings[GENET_DESC_INDEX];
		tx_reclaim(tx_ring);

		m_TxSpinLock.Release ();
	}
}

// handle Rx and Tx priority queues
void CBcm54213Device::InterruptHandler1 (void)
{
	// Read irq status
	unsigned status =  intrl2_1_readl(INTRL2_CPU_STAT) &
			  ~intrl2_1_readl(INTRL2_CPU_MASK_STATUS);

	// clear interrupts
	intrl2_1_writel(status, INTRL2_CPU_CLEAR);

	m_TxSpinLock.Acquire ();

	// Check Tx priority queue interrupts
	for (unsigned index = 0; index < TX_QUEUES; index++) {
		if (!(status & BIT(index)))
			continue;

		TGEnetTxRing *tx_ring = &m_tx_rings[index];
		tx_reclaim(tx_ring);
	}

	m_TxSpinLock.Release ();
}

void CBcm54213Device::InterruptStub0 (void *pParam)
{
	CBcm54213Device *pThis = (CBcm54213Device *) pParam;
	assert (pThis != 0);

	pThis->InterruptHandler0 ();
}

void CBcm54213Device::InterruptStub1 (void *pParam)
{
	CBcm54213Device *pThis = (CBcm54213Device *) pParam;
	assert (pThis != 0);

	pThis->InterruptHandler1 ();
}

int CBcm54213Device::mii_probe(void)
{
	// Initialize link state variables that mii_setup() uses
	m_old_link = -1;
	m_old_speed = -1;
	m_old_duplex = -1;
	m_old_pause = -1;

	// probe PHY
	m_phy_id = 0x01;
	int ret = mdio_reset();
	if (ret) {
		m_phy_id = 0x00;
		ret = mdio_reset();
	}
	if (ret)
		return ret;

	ret = phy_read_status();
	if (ret)
		return ret;

	mii_setup();

	return mii_config(true);		// Configure port multiplexer
}

// setup netdev link state when PHY link status change and
// update UMAC and RGMII block when link up
void CBcm54213Device::mii_setup(void)
{
	bool status_changed = false;

	if (m_old_link != m_link) {
		status_changed = true;
		m_old_link = m_link;
	}

	if (!m_link)
		return;

	// check speed/duplex/pause changes
	if (m_old_speed != m_speed) {
		status_changed = true;
		m_old_speed = m_speed;
	}

	if (m_old_duplex != m_duplex) {
		status_changed = true;
		m_old_duplex = m_duplex;
	}

	if (m_old_pause != m_pause) {
		status_changed = true;
		m_old_pause = m_pause;
	}

	// done if nothing has changed
	if (!status_changed)
		return;

	// speed
	u32 cmd_bits = 0;
	if (m_speed == 1000)
		cmd_bits = UMAC_SPEED_1000;
	else if (m_speed == 100)
		cmd_bits = UMAC_SPEED_100;
	else
		cmd_bits = UMAC_SPEED_10;
	cmd_bits <<= CMD_SPEED_SHIFT;

	// duplex
	if (!m_duplex)
		cmd_bits |= CMD_HD_EN;

	// pause capability
	if (!m_pause)
		cmd_bits |= CMD_RX_PAUSE_IGNORE | CMD_TX_PAUSE_IGNORE;

	// Program UMAC and RGMII block based on established
	// link speed, duplex, and pause. The speed set in
	// umac->cmd tell RGMII block which clock to use for
	// transmit -- 25MHz(100Mbps) or 125MHz(1Gbps).
	// Receive clock is provided by the PHY.
	u32 reg = ext_readl(EXT_RGMII_OOB_CTRL);
	reg &= ~OOB_DISABLE;
	reg |= RGMII_LINK;
	ext_writel(reg, EXT_RGMII_OOB_CTRL);

	reg = umac_readl(UMAC_CMD);
	reg &= ~((CMD_SPEED_MASK << CMD_SPEED_SHIFT)
		| CMD_HD_EN
		| CMD_RX_PAUSE_IGNORE | CMD_TX_PAUSE_IGNORE);
	reg |= cmd_bits;
	umac_writel(reg, UMAC_CMD);
}

int CBcm54213Device::mii_config(bool init)
{
	// RGMII_NO_ID: TXC transitions at the same time as TXD
	//		(requires PCB or receiver-side delay)
	// RGMII:	Add 2ns delay on TXC (90 degree shift)
	//
	// ID is implicitly disabled for 100Mbps (RG)MII operation.
	u32 id_mode_dis = BIT(16);

	sys_writel(PORT_MODE_EXT_GPHY, SYS_PORT_CTRL);

	// This is an external PHY (xMII), so we need to enable the RGMII
	// block for the interface to work
	u32 reg = ext_readl(EXT_RGMII_OOB_CTRL);
	reg |= RGMII_MODE_EN | id_mode_dis;
	ext_writel(reg, EXT_RGMII_OOB_CTRL);

	return 0;
}

//
// UniMAC MDIO
//

#define MDIO_CMD		0x00		// same register as UMAC_MDIO_CMD

#define MII_BMSR		0x01
	#define BMSR_LSTATUS		0x0004
#define MII_ADVERTISE		0x04
#define MII_LPA			0x05
	#define LPA_10HALF		0x0020
	#define LPA_10FULL		0x0040
	#define LPA_100HALF		0x0080
	#define LPA_100FULL		0x0100
	#define LPA_PAUSE_CAP		0x0400
#define MII_CTRL1000		0x09
#define MII_STAT1000		0x0A
	#define LPA_1000MSFAIL		0x8000
	#define LPA_1000FULL		0x0800
	#define LPA_1000HALF		0x0400

static inline u32 mdio_readl(u32 offset)
{
	return read32(ARM_BCM54213_MDIO + offset);
}

static inline void mdio_writel(u32 val, u32 offset)
{
	write32(ARM_BCM54213_MDIO + offset, val);
}

static inline void mdio_start(void)
{
	u32 reg = mdio_readl(MDIO_CMD);
	reg |= MDIO_START_BUSY;
	mdio_writel(reg, MDIO_CMD);
}

// static inline unsigned mdio_busy(void)
// {
// 	return mdio_readl(MDIO_CMD) & MDIO_START_BUSY;
// }

// Workaround for integrated BCM7xxx Gigabit PHYs which have a problem with
// their internal MDIO management controller making them fail to successfully
// be read from or written to for the first transaction.  We insert a dummy
// BMSR read here to make sure that phy_get_device() and get_phy_id() can
// correctly read the PHY MII_PHYSID1/2 registers and successfully register a
// PHY device for this peripheral.
int CBcm54213Device::mdio_reset(void)
{
	int ret = mdio_read(MII_BMSR);
	if (ret < 0)
		return ret;

	return 0;
}

int CBcm54213Device::mdio_read(int reg)
{
	// Prepare the read operation
	u32 cmd =   MDIO_RD
		  | (m_phy_id << MDIO_PMD_SHIFT)
		  | (reg << MDIO_REG_SHIFT);
	mdio_writel(cmd, MDIO_CMD);

	mdio_start();

	mdio_wait();

	cmd = mdio_readl(MDIO_CMD);

	if (cmd & MDIO_READ_FAIL)
		return -1;

	return cmd & 0xFFFF;
}

void CBcm54213Device::mdio_write(int reg, u16 val)
{
	// Prepare the write operation
	u32 cmd =   MDIO_WR
		  | (m_phy_id << MDIO_PMD_SHIFT)
		  | (reg << MDIO_REG_SHIFT)
		  | (0xFFFF & val);
	mdio_writel(cmd, MDIO_CMD);

	mdio_start();

	mdio_wait ();
}

// This is not the default wait_func from the UniMAC MDIO driver,
// but a private function assigned by the GENET bcmmii module.
void CBcm54213Device::mdio_wait(void)
{
	assert (m_pTimer != 0);
	unsigned nStartTicks = m_pTimer->GetClockTicks ();

	do
	{
		if (m_pTimer->GetClockTicks ()-nStartTicks >= CLOCKHZ / 100)
		{
			break;
		}
	}
	while (umac_readl(UMAC_MDIO_CMD) & MDIO_START_BUSY);
}

// phy_read_status - check the link status and update current link state
//
// Description: Check the link, then figure out the current state
//   by comparing what we advertise with what the link partner
//   advertises.  Start by checking the gigabit possibilities,
//   then move on to 10/100.
int CBcm54213Device::phy_read_status(void)
{
	// Update the link status, return if there was an error
	int bmsr = mdio_read(MII_BMSR);
	if (bmsr < 0)
		return bmsr;

	if ((bmsr & BMSR_LSTATUS) == 0) {
		m_link = 0;
		return 0;
	} else
		m_link = 1;

	// Read autonegotiation status
	// NOTE: autonegotiation is enabled by firmware, not here

	int lpagb = mdio_read(MII_STAT1000);
	if (lpagb < 0)
		return lpagb;

	int ctrl1000 = mdio_read(MII_CTRL1000);
	if (ctrl1000 < 0)
		return ctrl1000;

	if (lpagb & LPA_1000MSFAIL) {
		CLogger::Get ()->Write (FromBcm54213, LogWarning,
					"Master/Slave resolution failed (0x%X)", ctrl1000);
		return -1;
	}

	int common_adv_gb = lpagb & ctrl1000 << 2;

	int lpa = mdio_read(MII_LPA);
	if (lpa < 0)
		return lpa;

	int adv = mdio_read(MII_ADVERTISE);
	if (adv < 0)
		return adv;

	int common_adv = lpa & adv;

	m_speed = 10;
	m_duplex = 0;
	m_pause = 0;

	if (common_adv_gb & (LPA_1000FULL | LPA_1000HALF)) {
		m_speed = 1000;

		if (common_adv_gb & LPA_1000FULL)
			m_duplex = 1;
	} else if (common_adv & (LPA_100FULL | LPA_100HALF)) {
		m_speed = 100;

		if (common_adv & LPA_100FULL)
			m_duplex = 1;
	} else
		if (common_adv & LPA_10FULL)
			m_duplex = 1;

	if (m_duplex == 1) {
		m_pause = lpa & LPA_PAUSE_CAP ? 1 : 0;
	}

	return 0;
}
