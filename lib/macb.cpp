//
// macb.cpp
//
// This driver has been ported from the U-Boot driver:
//	drivers/net/macb.*
//	Copyright (C) 2005-2006 Atmel Corporation
//	SPDX-License-Identifier: GPL-2.0+
//
// Ported to Raspberry Pi 5 by aniplay62@GitHub
// Ported to Circle by R. Stange
//
#include <circle/macb.h>
#include <circle/memio.h>
#include <circle/bcm2712.h>
#include <circle/synchronize.h>
#include <circle/bcmpciehostbridge.h>
#include <circle/devicetreeblob.h>
#include <circle/machineinfo.h>
#include <circle/timer.h>
#include <circle/memory.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <assert.h>

#define PCLK_RATE		200000000UL
#define GPIO_PHY_RESET		32

#define DMA_BURST_LENGTH	16

#define MACB_GREGS_NBR		16
#define MACB_GREGS_VERSION	2
#define MACB_MAX_QUEUES		8

/* MACB register offsets */
#define MACB_NCR		0x0000 /* Network Control */
#define MACB_NCFGR		0x0004 /* Network Config */
#define MACB_NSR		0x0008 /* Network Status */
#define MACB_TAR		0x000c /* AT91RM9200 only */
#define MACB_TCR		0x0010 /* AT91RM9200 only */
#define MACB_TSR		0x0014 /* Transmit Status */
#define MACB_RBQP		0x0018 /* RX Q Base Address */
#define MACB_TBQP		0x001c /* TX Q Base Address */
#define MACB_RSR		0x0020 /* Receive Status */
#define MACB_ISR		0x0024 /* Interrupt Status */
#define MACB_IER		0x0028 /* Interrupt Enable */
#define MACB_IDR		0x002c /* Interrupt Disable */
#define MACB_IMR		0x0030 /* Interrupt Mask */
#define MACB_MAN		0x0034 /* PHY Maintenance */
#define MACB_PTR		0x0038
#define MACB_PFR		0x003c
#define MACB_FTO		0x0040
#define MACB_SCF		0x0044
#define MACB_MCF		0x0048
#define MACB_FRO		0x004c
#define MACB_FCSE		0x0050
#define MACB_ALE		0x0054
#define MACB_DTF		0x0058
#define MACB_LCOL		0x005c
#define MACB_EXCOL		0x0060
#define MACB_TUND		0x0064
#define MACB_CSE		0x0068
#define MACB_RRE		0x006c
#define MACB_ROVR		0x0070
#define MACB_RSE		0x0074
#define MACB_ELE		0x0078
#define MACB_RJA		0x007c
#define MACB_USF		0x0080
#define MACB_STE		0x0084
#define MACB_RLE		0x0088
#define MACB_TPF		0x008c
#define MACB_HRB		0x0090
#define MACB_HRT		0x0094
#define MACB_SA1B		0x0098
#define MACB_SA1T		0x009c
#define MACB_SA2B		0x00a0
#define MACB_SA2T		0x00a4
#define MACB_SA3B		0x00a8
#define MACB_SA3T		0x00ac
#define MACB_SA4B		0x00b0
#define MACB_SA4T		0x00b4
#define MACB_TID		0x00b8
#define MACB_TPQ		0x00bc
#define MACB_USRIO		0x00c0
#define MACB_WOL		0x00c4
#define MACB_MID		0x00fc
#define MACB_TBQPH		0x04C8
#define MACB_RBQPH		0x04D4

/* GEM register offsets. */
#define GEM_NCFGR		0x0004 /* Network Config */
#define GEM_USRIO		0x000c /* User IO */
#define GEM_DMACFG		0x0010 /* DMA Configuration */
#define GEM_JML			0x0048 /* Jumbo Max Length */
#define GEM_HRB			0x0080 /* Hash Bottom */
#define GEM_HRT			0x0084 /* Hash Top */
#define GEM_SA1B		0x0088 /* Specific1 Bottom */
#define GEM_SA1T		0x008C /* Specific1 Top */
#define GEM_SA2B		0x0090 /* Specific2 Bottom */
#define GEM_SA2T		0x0094 /* Specific2 Top */
#define GEM_SA3B		0x0098 /* Specific3 Bottom */
#define GEM_SA3T		0x009C /* Specific3 Top */
#define GEM_SA4B		0x00A0 /* Specific4 Bottom */
#define GEM_SA4T		0x00A4 /* Specific4 Top */
#define GEM_EFTSH		0x00e8 /* PTP Event Frame Transmitted Seconds Register 47:32 */
#define GEM_EFRSH		0x00ec /* PTP Event Frame Received Seconds Register 47:32 */
#define GEM_PEFTSH		0x00f0 /* PTP Peer Event Frame Transmitted Seconds Register 47:32 */
#define GEM_PEFRSH		0x00f4 /* PTP Peer Event Frame Received Seconds Register 47:32 */
#define GEM_OTX			0x0100 /* Octets transmitted */
#define GEM_OCTTXL		0x0100 /* Octets transmitted [31:0] */
#define GEM_OCTTXH		0x0104 /* Octets transmitted [47:32] */
#define GEM_TXCNT		0x0108 /* Frames Transmitted counter */
#define GEM_TXBCCNT		0x010c /* Broadcast Frames counter */
#define GEM_TXMCCNT		0x0110 /* Multicast Frames counter */
#define GEM_TXPAUSECNT		0x0114 /* Pause Frames Transmitted Counter */
#define GEM_TX64CNT		0x0118 /* 64 byte Frames TX counter */
#define GEM_TX65CNT		0x011c /* 65-127 byte Frames TX counter */
#define GEM_TX128CNT		0x0120 /* 128-255 byte Frames TX counter */
#define GEM_TX256CNT		0x0124 /* 256-511 byte Frames TX counter */
#define GEM_TX512CNT		0x0128 /* 512-1023 byte Frames TX counter */
#define GEM_TX1024CNT		0x012c /* 1024-1518 byte Frames TX counter */
#define GEM_TX1519CNT		0x0130 /* 1519+ byte Frames TX counter */
#define GEM_TXURUNCNT		0x0134 /* TX under run error counter */
#define GEM_SNGLCOLLCNT		0x0138 /* Single Collision Frame Counter */
#define GEM_MULTICOLLCNT	0x013c /* Multiple Collision Frame Counter */
#define GEM_EXCESSCOLLCNT	0x0140 /* Excessive Collision Frame Counter */
#define GEM_LATECOLLCNT		0x0144 /* Late Collision Frame Counter */
#define GEM_TXDEFERCNT		0x0148 /* Deferred Transmission Frame Counter */
#define GEM_TXCSENSECNT		0x014c /* Carrier Sense Error Counter */
#define GEM_ORX			0x0150 /* Octets received */
#define GEM_OCTRXL		0x0150 /* Octets received [31:0] */
#define GEM_OCTRXH		0x0154 /* Octets received [47:32] */
#define GEM_RXCNT		0x0158 /* Frames Received Counter */
#define GEM_RXBROADCNT		0x015c /* Broadcast Frames Received Counter */
#define GEM_RXMULTICNT		0x0160 /* Multicast Frames Received Counter */
#define GEM_RXPAUSECNT		0x0164 /* Pause Frames Received Counter */
#define GEM_RX64CNT		0x0168 /* 64 byte Frames RX Counter */
#define GEM_RX65CNT		0x016c /* 65-127 byte Frames RX Counter */
#define GEM_RX128CNT		0x0170 /* 128-255 byte Frames RX Counter */
#define GEM_RX256CNT		0x0174 /* 256-511 byte Frames RX Counter */
#define GEM_RX512CNT		0x0178 /* 512-1023 byte Frames RX Counter */
#define GEM_RX1024CNT		0x017c /* 1024-1518 byte Frames RX Counter */
#define GEM_RX1519CNT		0x0180 /* 1519+ byte Frames RX Counter */
#define GEM_RXUNDRCNT		0x0184 /* Undersize Frames Received Counter */
#define GEM_RXOVRCNT		0x0188 /* Oversize Frames Received Counter */
#define GEM_RXJABCNT		0x018c /* Jabbers Received Counter */
#define GEM_RXFCSCNT		0x0190 /* Frame Check Sequence Error Counter */
#define GEM_RXLENGTHCNT		0x0194 /* Length Field Error Counter */
#define GEM_RXSYMBCNT		0x0198 /* Symbol Error Counter */
#define GEM_RXALIGNCNT		0x019c /* Alignment Error Counter */
#define GEM_RXRESERRCNT		0x01a0 /* Receive Resource Error Counter */
#define GEM_RXORCNT		0x01a4 /* Receive Overrun Counter */
#define GEM_RXIPCCNT		0x01a8 /* IP header Checksum Error Counter */
#define GEM_RXTCPCCNT		0x01ac /* TCP Checksum Error Counter */
#define GEM_RXUDPCCNT		0x01b0 /* UDP Checksum Error Counter */
#define GEM_TISUBN		0x01bc /* 1588 Timer Increment Sub-ns */
#define GEM_TSH			0x01c0 /* 1588 Timer Seconds High */
#define GEM_TSL			0x01d0 /* 1588 Timer Seconds Low */
#define GEM_TN			0x01d4 /* 1588 Timer Nanoseconds */
#define GEM_TA			0x01d8 /* 1588 Timer Adjust */
#define GEM_TI			0x01dc /* 1588 Timer Increment */
#define GEM_EFTSL		0x01e0 /* PTP Event Frame Tx Seconds Low */
#define GEM_EFTN		0x01e4 /* PTP Event Frame Tx Nanoseconds */
#define GEM_EFRSL		0x01e8 /* PTP Event Frame Rx Seconds Low */
#define GEM_EFRN		0x01ec /* PTP Event Frame Rx Nanoseconds */
#define GEM_PEFTSL		0x01f0 /* PTP Peer Event Frame Tx Secs Low */
#define GEM_PEFTN		0x01f4 /* PTP Peer Event Frame Tx Ns */
#define GEM_PEFRSL		0x01f8 /* PTP Peer Event Frame Rx Sec Low */
#define GEM_PEFRN		0x01fc /* PTP Peer Event Frame Rx Ns */
#define GEM_DCFG1		0x0280 /* Design Config 1 */
#define GEM_DCFG2		0x0284 /* Design Config 2 */
#define GEM_DCFG3		0x0288 /* Design Config 3 */
#define GEM_DCFG4		0x028c /* Design Config 4 */
#define GEM_DCFG5		0x0290 /* Design Config 5 */
#define GEM_DCFG6		0x0294 /* Design Config 6 */
#define GEM_DCFG7		0x0298 /* Design Config 7 */
#define GEM_DCFG8		0x029C /* Design Config 8 */
#define GEM_DCFG10		0x02A4 /* Design Config 10 */

#define GEM_TXBDCTRL		0x04cc /* TX Buffer Descriptor control register */
#define GEM_RXBDCTRL		0x04d0 /* RX Buffer Descriptor control register */

/* Screener Type 2 match registers */
#define GEM_SCRT2		0x540

/* EtherType registers */
#define GEM_ETHT		0x06E0

/* Type 2 compare registers */
#define GEM_T2CMPW0		0x0700
#define GEM_T2CMPW1		0x0704
#define T2CMP_OFST(t2idx)	(t2idx * 2)

/* type 2 compare registers
 * each location requires 3 compare regs
 */
#define GEM_IP4SRC_CMP(idx)	(idx * 3)
#define GEM_IP4DST_CMP(idx)	(idx * 3 + 1)
#define GEM_PORT_CMP(idx)	(idx * 3 + 2)

/* Which screening type 2 EtherType register will be used (0 - 7) */
#define SCRT2_ETHT		0

#define GEM_ISR(hw_q)		(0x0400 + ((hw_q) << 2))
#define GEM_TBQP(hw_q)		(0x0440 + ((hw_q) << 2))
#define GEM_TBQPH(hw_q)		(0x04C8)
#define GEM_RBQP(hw_q)		(0x0480 + ((hw_q) << 2))
#define GEM_RBQS(hw_q)		(0x04A0 + ((hw_q) << 2))
#define GEM_RBQPH(hw_q)		(0x04D4)
#define GEM_IER(hw_q)		(0x0600 + ((hw_q) << 2))
#define GEM_IDR(hw_q)		(0x0620 + ((hw_q) << 2))
#define GEM_IMR(hw_q)		(0x0640 + ((hw_q) << 2))

/* Bitfields in NCR */
#define MACB_LB_OFFSET		0 /* reserved */
#define MACB_LB_SIZE		1
#define MACB_LLB_OFFSET		1 /* Loop back local */
#define MACB_LLB_SIZE		1
#define MACB_RE_OFFSET		2 /* Receive enable */
#define MACB_RE_SIZE		1
#define MACB_TE_OFFSET		3 /* Transmit enable */
#define MACB_TE_SIZE		1
#define MACB_MPE_OFFSET		4 /* Management port enable */
#define MACB_MPE_SIZE		1
#define MACB_CLRSTAT_OFFSET	5 /* Clear stats regs */
#define MACB_CLRSTAT_SIZE	1
#define MACB_INCSTAT_OFFSET	6 /* Incremental stats regs */
#define MACB_INCSTAT_SIZE	1
#define MACB_WESTAT_OFFSET	7 /* Write enable stats regs */
#define MACB_WESTAT_SIZE	1
#define MACB_BP_OFFSET		8 /* Back pressure */
#define MACB_BP_SIZE		1
#define MACB_TSTART_OFFSET	9 /* Start transmission */
#define MACB_TSTART_SIZE	1
#define MACB_THALT_OFFSET	10 /* Transmit halt */
#define MACB_THALT_SIZE		1
#define MACB_NCR_TPF_OFFSET	11 /* Transmit pause frame */
#define MACB_NCR_TPF_SIZE	1
#define MACB_TZQ_OFFSET		12 /* Transmit zero quantum pause frame */
#define MACB_TZQ_SIZE		1
#define MACB_SRTSM_OFFSET	15
#define MACB_OSSMODE_OFFSET 24 /* Enable One Step Synchro Mode */
#define MACB_OSSMODE_SIZE	1

/* Bitfields in NCFGR */
#define MACB_SPD_OFFSET		0 /* Speed */
#define MACB_SPD_SIZE		1
#define MACB_FD_OFFSET		1 /* Full duplex */
#define MACB_FD_SIZE		1
#define MACB_BIT_RATE_OFFSET	2 /* Discard non-VLAN frames */
#define MACB_BIT_RATE_SIZE	1
#define MACB_JFRAME_OFFSET	3 /* reserved */
#define MACB_JFRAME_SIZE	1
#define MACB_CAF_OFFSET		4 /* Copy all frames */
#define MACB_CAF_SIZE		1
#define MACB_NBC_OFFSET		5 /* No broadcast */
#define MACB_NBC_SIZE		1
#define MACB_NCFGR_MTI_OFFSET	6 /* Multicast hash enable */
#define MACB_NCFGR_MTI_SIZE	1
#define MACB_UNI_OFFSET		7 /* Unicast hash enable */
#define MACB_UNI_SIZE		1
#define MACB_BIG_OFFSET		8 /* Receive 1536 byte frames */
#define MACB_BIG_SIZE		1
#define MACB_EAE_OFFSET		9 /* External address match enable */
#define MACB_EAE_SIZE		1
#define MACB_CLK_OFFSET		10
#define MACB_CLK_SIZE		2
#define MACB_RTY_OFFSET		12 /* Retry test */
#define MACB_RTY_SIZE		1
#define MACB_PAE_OFFSET		13 /* Pause enable */
#define MACB_PAE_SIZE		1
#define MACB_RM9200_RMII_OFFSET	13 /* AT91RM9200 only */
#define MACB_RM9200_RMII_SIZE	1  /* AT91RM9200 only */
#define MACB_RBOF_OFFSET	14 /* Receive buffer offset */
#define MACB_RBOF_SIZE		2
#define MACB_RLCE_OFFSET	16 /* Length field error frame discard */
#define MACB_RLCE_SIZE		1
#define MACB_DRFCS_OFFSET	17 /* FCS remove */
#define MACB_DRFCS_SIZE		1
#define MACB_EFRHD_OFFSET	18
#define MACB_EFRHD_SIZE		1
#define MACB_IRXFCS_OFFSET	19
#define MACB_IRXFCS_SIZE	1

/* GEM specific NCFGR bitfields. */
#define GEM_GBE_OFFSET		10 /* Gigabit mode enable */
#define GEM_GBE_SIZE		1
#define GEM_PCSSEL_OFFSET	11
#define GEM_PCSSEL_SIZE		1
#define GEM_CLK_OFFSET		18 /* MDC clock division */
#define GEM_CLK_SIZE		3
#define GEM_DBW_OFFSET		21 /* Data bus width */
#define GEM_DBW_SIZE		2
#define GEM_RXCOEN_OFFSET	24
#define GEM_RXCOEN_SIZE		1
#define GEM_SGMIIEN_OFFSET	27
#define GEM_SGMIIEN_SIZE	1


/* Constants for data bus width. */
#define GEM_DBW32		0 /* 32 bit AMBA AHB data bus width */
#define GEM_DBW64		1 /* 64 bit AMBA AHB data bus width */
#define GEM_DBW128		2 /* 128 bit AMBA AHB data bus width */

/* Bitfields in DMACFG. */
#define GEM_FBLDO_OFFSET	0 /* fixed burst length for DMA */
#define GEM_FBLDO_SIZE		5
#define GEM_ENDIA_DESC_OFFSET	6 /* endian swap mode for management descriptor access */
#define GEM_ENDIA_DESC_SIZE	1
#define GEM_ENDIA_PKT_OFFSET	7 /* endian swap mode for packet data access */
#define GEM_ENDIA_PKT_SIZE	1
#define GEM_RXBMS_OFFSET	8 /* RX packet buffer memory size select */
#define GEM_RXBMS_SIZE		2
#define GEM_TXPBMS_OFFSET	10 /* TX packet buffer memory size select */
#define GEM_TXPBMS_SIZE		1
#define GEM_TXCOEN_OFFSET	11 /* TX IP/TCP/UDP checksum gen offload */
#define GEM_TXCOEN_SIZE		1
#define GEM_RXBS_OFFSET		16 /* DMA receive buffer size */
#define GEM_RXBS_SIZE		8
#define GEM_DDRP_OFFSET		24 /* disc_when_no_ahb */
#define GEM_DDRP_SIZE		1
#define GEM_RXEXT_OFFSET	28 /* RX extended Buffer Descriptor mode */
#define GEM_RXEXT_SIZE		1
#define GEM_TXEXT_OFFSET	29 /* TX extended Buffer Descriptor mode */
#define GEM_TXEXT_SIZE		1
#define GEM_ADDR64_OFFSET	30 /* Address bus width - 64b or 32b */
#define GEM_ADDR64_SIZE		1


/* Bitfields in NSR */
#define MACB_NSR_LINK_OFFSET	0 /* pcs_link_state */
#define MACB_NSR_LINK_SIZE	1
#define MACB_MDIO_OFFSET	1 /* status of the mdio_in pin */
#define MACB_MDIO_SIZE		1
#define MACB_IDLE_OFFSET	2 /* The PHY management logic is idle */
#define MACB_IDLE_SIZE		1

/* Bitfields in TSR */
#define MACB_UBR_OFFSET		0 /* Used bit read */
#define MACB_UBR_SIZE		1
#define MACB_COL_OFFSET		1 /* Collision occurred */
#define MACB_COL_SIZE		1
#define MACB_TSR_RLE_OFFSET	2 /* Retry limit exceeded */
#define MACB_TSR_RLE_SIZE	1
#define MACB_TGO_OFFSET		3 /* Transmit go */
#define MACB_TGO_SIZE		1
#define MACB_BEX_OFFSET		4 /* TX frame corruption due to AHB error */
#define MACB_BEX_SIZE		1
#define MACB_RM9200_BNQ_OFFSET	4 /* AT91RM9200 only */
#define MACB_RM9200_BNQ_SIZE	1 /* AT91RM9200 only */
#define MACB_COMP_OFFSET	5 /* Trnasmit complete */
#define MACB_COMP_SIZE		1
#define MACB_UND_OFFSET		6 /* Trnasmit under run */
#define MACB_UND_SIZE		1

/* Bitfields in RSR */
#define MACB_BNA_OFFSET		0 /* Buffer not available */
#define MACB_BNA_SIZE		1
#define MACB_REC_OFFSET		1 /* Frame received */
#define MACB_REC_SIZE		1
#define MACB_OVR_OFFSET		2 /* Receive overrun */
#define MACB_OVR_SIZE		1

/* Bitfields in ISR/IER/IDR/IMR */
#define MACB_MFD_OFFSET		0 /* Management frame sent */
#define MACB_MFD_SIZE		1
#define MACB_RCOMP_OFFSET	1 /* Receive complete */
#define MACB_RCOMP_SIZE		1
#define MACB_RXUBR_OFFSET	2 /* RX used bit read */
#define MACB_RXUBR_SIZE		1
#define MACB_TXUBR_OFFSET	3 /* TX used bit read */
#define MACB_TXUBR_SIZE		1
#define MACB_ISR_TUND_OFFSET	4 /* Enable TX buffer under run interrupt */
#define MACB_ISR_TUND_SIZE	1
#define MACB_ISR_RLE_OFFSET	5 /* EN retry exceeded/late coll interrupt */
#define MACB_ISR_RLE_SIZE	1
#define MACB_TXERR_OFFSET	6 /* EN TX frame corrupt from error interrupt */
#define MACB_TXERR_SIZE		1
#define MACB_TCOMP_OFFSET	7 /* Enable transmit complete interrupt */
#define MACB_TCOMP_SIZE		1
#define MACB_ISR_LINK_OFFSET	9 /* Enable link change interrupt */
#define MACB_ISR_LINK_SIZE	1
#define MACB_ISR_ROVR_OFFSET	10 /* Enable receive overrun interrupt */
#define MACB_ISR_ROVR_SIZE	1
#define MACB_HRESP_OFFSET	11 /* Enable hrsep not OK interrupt */
#define MACB_HRESP_SIZE		1
#define MACB_PFR_OFFSET		12 /* Enable pause frame w/ quantum interrupt */
#define MACB_PFR_SIZE		1
#define MACB_PTZ_OFFSET		13 /* Enable pause time zero interrupt */
#define MACB_PTZ_SIZE		1
#define MACB_WOL_OFFSET		14 /* Enable wake-on-lan interrupt */
#define MACB_WOL_SIZE		1
#define MACB_DRQFR_OFFSET	18 /* PTP Delay Request Frame Received */
#define MACB_DRQFR_SIZE		1
#define MACB_SFR_OFFSET		19 /* PTP Sync Frame Received */
#define MACB_SFR_SIZE		1
#define MACB_DRQFT_OFFSET	20 /* PTP Delay Request Frame Transmitted */
#define MACB_DRQFT_SIZE		1
#define MACB_SFT_OFFSET		21 /* PTP Sync Frame Transmitted */
#define MACB_SFT_SIZE		1
#define MACB_PDRQFR_OFFSET	22 /* PDelay Request Frame Received */
#define MACB_PDRQFR_SIZE	1
#define MACB_PDRSFR_OFFSET	23 /* PDelay Response Frame Received */
#define MACB_PDRSFR_SIZE	1
#define MACB_PDRQFT_OFFSET	24 /* PDelay Request Frame Transmitted */
#define MACB_PDRQFT_SIZE	1
#define MACB_PDRSFT_OFFSET	25 /* PDelay Response Frame Transmitted */
#define MACB_PDRSFT_SIZE	1
#define MACB_SRI_OFFSET		26 /* TSU Seconds Register Increment */
#define MACB_SRI_SIZE		1

/* Timer increment fields */
#define MACB_TI_CNS_OFFSET	0
#define MACB_TI_CNS_SIZE	8
#define MACB_TI_ACNS_OFFSET	8
#define MACB_TI_ACNS_SIZE	8
#define MACB_TI_NIT_OFFSET	16
#define MACB_TI_NIT_SIZE	8

/* Bitfields in MAN */
#define MACB_DATA_OFFSET	0 /* data */
#define MACB_DATA_SIZE		16
#define MACB_CODE_OFFSET	16 /* Must be written to 10 */
#define MACB_CODE_SIZE		2
#define MACB_REGA_OFFSET	18 /* Register address */
#define MACB_REGA_SIZE		5
#define MACB_PHYA_OFFSET	23 /* PHY address */
#define MACB_PHYA_SIZE		5
#define MACB_RW_OFFSET		28 /* Operation. 10 is read. 01 is write. */
#define MACB_RW_SIZE		2
#define MACB_SOF_OFFSET		30 /* Must be written to 1 for Clause 22 */
#define MACB_SOF_SIZE		2

/* Bitfields in USRIO (AVR32) */
#define MACB_MII_OFFSET		0
#define MACB_MII_SIZE		1
#define MACB_EAM_OFFSET		1
#define MACB_EAM_SIZE		1
#define MACB_TX_PAUSE_OFFSET	2
#define MACB_TX_PAUSE_SIZE	1
#define MACB_TX_PAUSE_ZERO_OFFSET 3
#define MACB_TX_PAUSE_ZERO_SIZE	1

/* Bitfields in USRIO (AT91) */
#define MACB_RMII_OFFSET	0
#define MACB_RMII_SIZE		1
#define GEM_RGMII_OFFSET	0 /* GEM gigabit mode */
#define GEM_RGMII_SIZE		1
#define MACB_CLKEN_OFFSET	1
#define MACB_CLKEN_SIZE		1

/* Bitfields in WOL */
#define MACB_IP_OFFSET		0
#define MACB_IP_SIZE		16
#define MACB_MAG_OFFSET		16
#define MACB_MAG_SIZE		1
#define MACB_ARP_OFFSET		17
#define MACB_ARP_SIZE		1
#define MACB_SA1_OFFSET		18
#define MACB_SA1_SIZE		1
#define MACB_WOL_MTI_OFFSET	19
#define MACB_WOL_MTI_SIZE	1

/* Bitfields in MID */
#define MACB_IDNUM_OFFSET	16
#define MACB_IDNUM_SIZE		12
#define MACB_REV_OFFSET		0
#define MACB_REV_SIZE		16

/* Bitfields in DCFG1. */
#define GEM_IRQCOR_OFFSET	23
#define GEM_IRQCOR_SIZE		1
#define GEM_DBWDEF_OFFSET	25
#define GEM_DBWDEF_SIZE		3

/* Bitfields in DCFG2. */
#define GEM_RX_PKT_BUFF_OFFSET	20
#define GEM_RX_PKT_BUFF_SIZE	1
#define GEM_TX_PKT_BUFF_OFFSET	21
#define GEM_TX_PKT_BUFF_SIZE	1


/* Bitfields in DCFG5. */
#define GEM_TSU_OFFSET		8
#define GEM_TSU_SIZE		1

/* Bitfields in DCFG6. */
#define GEM_PBUF_LSO_OFFSET	27
#define GEM_PBUF_LSO_SIZE	1
#define GEM_DAW64_OFFSET	23
#define GEM_DAW64_SIZE		1

/* Bitfields in DCFG8. */
#define GEM_T1SCR_OFFSET	24
#define GEM_T1SCR_SIZE		8
#define GEM_T2SCR_OFFSET	16
#define GEM_T2SCR_SIZE		8
#define GEM_SCR2ETH_OFFSET	8
#define GEM_SCR2ETH_SIZE	8
#define GEM_SCR2CMP_OFFSET	0
#define GEM_SCR2CMP_SIZE	8

/* Bitfields in DCFG10 */
#define GEM_TXBD_RDBUFF_OFFSET	12
#define GEM_TXBD_RDBUFF_SIZE	4
#define GEM_RXBD_RDBUFF_OFFSET	8
#define GEM_RXBD_RDBUFF_SIZE	4

/* Bitfields in TISUBN */
#define GEM_SUBNSINCR_OFFSET	0
#define GEM_SUBNSINCR_SIZE	16

/* Bitfields in TI */
#define GEM_NSINCR_OFFSET	0
#define GEM_NSINCR_SIZE		8

/* Bitfields in TSH */
#define GEM_TSH_OFFSET		0 /* TSU timer value (s). MSB [47:32] of seconds timer count */
#define GEM_TSH_SIZE		16

/* Bitfields in TSL */
#define GEM_TSL_OFFSET		0 /* TSU timer value (s). LSB [31:0] of seconds timer count */
#define GEM_TSL_SIZE		32

/* Bitfields in TN */
#define GEM_TN_OFFSET		0 /* TSU timer value (ns) */
#define GEM_TN_SIZE			30

/* Bitfields in TXBDCTRL */
#define GEM_TXTSMODE_OFFSET	4 /* TX Descriptor Timestamp Insertion mode */
#define GEM_TXTSMODE_SIZE	2

/* Bitfields in RXBDCTRL */
#define GEM_RXTSMODE_OFFSET	4 /* RX Descriptor Timestamp Insertion mode */
#define GEM_RXTSMODE_SIZE	2

/* Bitfields in SCRT2 */
#define GEM_QUEUE_OFFSET	0 /* Queue Number */
#define GEM_QUEUE_SIZE		4
#define GEM_VLANPR_OFFSET	4 /* VLAN Priority */
#define GEM_VLANPR_SIZE		3
#define GEM_VLANEN_OFFSET	8 /* VLAN Enable */
#define GEM_VLANEN_SIZE		1
#define GEM_ETHT2IDX_OFFSET	9 /* Index to screener type 2 EtherType register */
#define GEM_ETHT2IDX_SIZE	3
#define GEM_ETHTEN_OFFSET	12 /* EtherType Enable */
#define GEM_ETHTEN_SIZE		1
#define GEM_CMPA_OFFSET		13 /* Compare A - Index to screener type 2 Compare register */
#define GEM_CMPA_SIZE		5
#define GEM_CMPAEN_OFFSET	18 /* Compare A Enable */
#define GEM_CMPAEN_SIZE		1
#define GEM_CMPB_OFFSET		19 /* Compare B - Index to screener type 2 Compare register */
#define GEM_CMPB_SIZE		5
#define GEM_CMPBEN_OFFSET	24 /* Compare B Enable */
#define GEM_CMPBEN_SIZE		1
#define GEM_CMPC_OFFSET		25 /* Compare C - Index to screener type 2 Compare register */
#define GEM_CMPC_SIZE		5
#define GEM_CMPCEN_OFFSET	30 /* Compare C Enable */
#define GEM_CMPCEN_SIZE		1

/* Bitfields in ETHT */
#define GEM_ETHTCMP_OFFSET	0 /* EtherType compare value */
#define GEM_ETHTCMP_SIZE	16

/* Bitfields in T2CMPW0 */
#define GEM_T2CMP_OFFSET	16 /* 0xFFFF0000 compare value */
#define GEM_T2CMP_SIZE		16
#define GEM_T2MASK_OFFSET	0 /* 0x0000FFFF compare value or mask */
#define GEM_T2MASK_SIZE		16

/* Bitfields in T2CMPW1 */
#define GEM_T2DISMSK_OFFSET	9 /* disable mask */
#define GEM_T2DISMSK_SIZE	1
#define GEM_T2CMPOFST_OFFSET	7 /* compare offset */
#define GEM_T2CMPOFST_SIZE	2
#define GEM_T2OFST_OFFSET	0 /* offset value */
#define GEM_T2OFST_SIZE		7

/* Offset for screener type 2 compare values (T2CMPOFST).
 * Note the offset is applied after the specified point,
 * e.g. GEM_T2COMPOFST_ETYPE denotes the EtherType field, so an offset
 * of 12 bytes from this would be the source IP address in an IP header
 */
#define GEM_T2COMPOFST_SOF	0
#define GEM_T2COMPOFST_ETYPE	1
#define GEM_T2COMPOFST_IPHDR	2
#define GEM_T2COMPOFST_TCPUDP	3

/* offset from EtherType to IP address */
#define ETYPE_SRCIP_OFFSET	12
#define ETYPE_DSTIP_OFFSET	16

/* offset from IP header to port */
#define IPHDR_SRCPORT_OFFSET	0
#define IPHDR_DSTPORT_OFFSET	2

/* Transmit DMA buffer descriptor Word 1 */
#define GEM_DMA_TXVALID_OFFSET	23 /* timestamp has been captured in the Buffer Descriptor */
#define GEM_DMA_TXVALID_SIZE	1

/* Receive DMA buffer descriptor Word 0 */
#define GEM_DMA_RXVALID_OFFSET	2 /* indicates a valid timestamp in the Buffer Descriptor */
#define GEM_DMA_RXVALID_SIZE	1

/* DMA buffer descriptor Word 2 (32 bit addressing) or Word 4 (64 bit addressing) */
#define GEM_DMA_SECL_OFFSET	30 /* Timestamp seconds[1:0]  */
#define GEM_DMA_SECL_SIZE	2
#define GEM_DMA_NSEC_OFFSET	0 /* Timestamp nanosecs [29:0] */
#define GEM_DMA_NSEC_SIZE	30

/* DMA buffer descriptor Word 3 (32 bit addressing) or Word 5 (64 bit addressing) */

/* New hardware supports 12 bit precision of timestamp in DMA buffer descriptor.
 * Old hardware supports only 6 bit precision but it is enough for PTP.
 * Less accuracy is used always instead of checking hardware version.
 */
#define GEM_DMA_SECH_OFFSET	0 /* Timestamp seconds[5:2] */
#define GEM_DMA_SECH_SIZE	4
#define GEM_DMA_SEC_WIDTH	(GEM_DMA_SECH_SIZE + GEM_DMA_SECL_SIZE)
#define GEM_DMA_SEC_TOP		(1 << GEM_DMA_SEC_WIDTH)
#define GEM_DMA_SEC_MASK	(GEM_DMA_SEC_TOP - 1)

/* Bitfields in ADJ */
#define GEM_ADDSUB_OFFSET	31
#define GEM_ADDSUB_SIZE		1
/* Constants for CLK */
#define MACB_CLK_DIV8		0
#define MACB_CLK_DIV16		1
#define MACB_CLK_DIV32		2
#define MACB_CLK_DIV64		3

/* GEM specific constants for CLK */
#define GEM_CLK_DIV8		0
#define GEM_CLK_DIV16		1
#define GEM_CLK_DIV32		2
#define GEM_CLK_DIV48		3
#define GEM_CLK_DIV64		4
#define GEM_CLK_DIV96		5
#define GEM_CLK_DIV128		6
#define GEM_CLK_DIV224		7

/* Constants for MAN register */
#define MACB_MAN_SOF		1
#define MACB_MAN_WRITE		1
#define MACB_MAN_READ		2
#define MACB_MAN_CODE		2

/* Capability mask bits */
#define MACB_CAPS_ISR_CLEAR_ON_WRITE		0x00000001
#define MACB_CAPS_USRIO_HAS_CLKEN		0x00000002
#define MACB_CAPS_USRIO_DEFAULT_IS_MII_GMII	0x00000004
#define MACB_CAPS_NO_GIGABIT_HALF		0x00000008
#define MACB_CAPS_USRIO_DISABLED		0x00000010
#define MACB_CAPS_JUMBO				0x00000020
#define MACB_CAPS_GEM_HAS_PTP			0x00000040
#define MACB_CAPS_BD_RD_PREFETCH		0x00000080
#define MACB_CAPS_NEEDS_RSTONUBR		0x00000100
#define MACB_CAPS_FIFO_MODE			0x10000000
#define MACB_CAPS_GIGABIT_MODE_AVAILABLE	0x20000000
#define MACB_CAPS_SG_DISABLED			0x40000000
#define MACB_CAPS_MACB_IS_GEM			0x80000000

/* LSO settings */
#define MACB_LSO_UFO_ENABLE			0x01
#define MACB_LSO_TSO_ENABLE			0x02

/* Bit manipulation macros */
#define MACB_BIT(name)					\
	(1 << MACB_##name##_OFFSET)
#define MACB_BF(name,value)				\
	(((value) & ((1 << MACB_##name##_SIZE) - 1))	\
	 << MACB_##name##_OFFSET)
#define MACB_BFEXT(name,value)				\
	(((value) >> MACB_##name##_OFFSET)		\
	 & ((1 << MACB_##name##_SIZE) - 1))
#define MACB_BFINS(name,value,old)			\
	(((old) & ~(((1 << MACB_##name##_SIZE) - 1)	\
		    << MACB_##name##_OFFSET))		\
	 | MACB_BF(name,value))

#define GEM_BIT(name)					\
	(1 << GEM_##name##_OFFSET)
#define GEM_BF(name, value)				\
	(((value) & ((1 << GEM_##name##_SIZE) - 1))	\
	 << GEM_##name##_OFFSET)
#define GEM_BFEXT(name, value)				\
	(((value) >> GEM_##name##_OFFSET)		\
	 & ((1 << GEM_##name##_SIZE) - 1))
#define GEM_BFINS(name, value, old)			\
	(((old) & ~(((1 << GEM_##name##_SIZE) - 1)	\
		    << GEM_##name##_OFFSET))		\
	 | GEM_BF(name, value))

/* Register access macros */
#define macb_readl(reg)					\
	read32(ARM_MACB_BASE + MACB_##reg)
#define macb_writel(reg, value)				\
	write32(ARM_MACB_BASE + MACB_##reg, (value))
#define gem_readl(reg)					\
	read32(ARM_MACB_BASE + GEM_##reg)
#define gem_writel(reg, value)				\
	write32(ARM_MACB_BASE + GEM_##reg, (value))

/* DMA descriptor bitfields */
#define MACB_RX_USED_OFFSET			0
#define MACB_RX_USED_SIZE			1
#define MACB_RX_WRAP_OFFSET			1
#define MACB_RX_WRAP_SIZE			1
#define MACB_RX_WADDR_OFFSET			2
#define MACB_RX_WADDR_SIZE			30

#define MACB_RX_FRMLEN_OFFSET			0
#define MACB_RX_FRMLEN_SIZE			12
#define MACB_RX_OFFSET_OFFSET			12
#define MACB_RX_OFFSET_SIZE			2
#define MACB_RX_SOF_OFFSET			14
#define MACB_RX_SOF_SIZE			1
#define MACB_RX_EOF_OFFSET			15
#define MACB_RX_EOF_SIZE			1
#define MACB_RX_CFI_OFFSET			16
#define MACB_RX_CFI_SIZE			1
#define MACB_RX_VLAN_PRI_OFFSET			17
#define MACB_RX_VLAN_PRI_SIZE			3
#define MACB_RX_PRI_TAG_OFFSET			20
#define MACB_RX_PRI_TAG_SIZE			1
#define MACB_RX_VLAN_TAG_OFFSET			21
#define MACB_RX_VLAN_TAG_SIZE			1
#define MACB_RX_TYPEID_MATCH_OFFSET		22
#define MACB_RX_TYPEID_MATCH_SIZE		1
#define MACB_RX_SA4_MATCH_OFFSET		23
#define MACB_RX_SA4_MATCH_SIZE			1
#define MACB_RX_SA3_MATCH_OFFSET		24
#define MACB_RX_SA3_MATCH_SIZE			1
#define MACB_RX_SA2_MATCH_OFFSET		25
#define MACB_RX_SA2_MATCH_SIZE			1
#define MACB_RX_SA1_MATCH_OFFSET		26
#define MACB_RX_SA1_MATCH_SIZE			1
#define MACB_RX_EXT_MATCH_OFFSET		28
#define MACB_RX_EXT_MATCH_SIZE			1
#define MACB_RX_UHASH_MATCH_OFFSET		29
#define MACB_RX_UHASH_MATCH_SIZE		1
#define MACB_RX_MHASH_MATCH_OFFSET		30
#define MACB_RX_MHASH_MATCH_SIZE		1
#define MACB_RX_BROADCAST_OFFSET		31
#define MACB_RX_BROADCAST_SIZE			1

#define MACB_RX_FRMLEN_MASK			0xFFF
#define MACB_RX_JFRMLEN_MASK			0x3FFF

/* RX checksum offload disabled: bit 24 clear in NCFGR */
#define GEM_RX_TYPEID_MATCH_OFFSET		22
#define GEM_RX_TYPEID_MATCH_SIZE		2

/* RX checksum offload enabled: bit 24 set in NCFGR */
#define GEM_RX_CSUM_OFFSET			22
#define GEM_RX_CSUM_SIZE			2

#define MACB_TX_FRMLEN_OFFSET			0
#define MACB_TX_FRMLEN_SIZE			11
#define MACB_TX_LAST_OFFSET			15
#define MACB_TX_LAST_SIZE			1
#define MACB_TX_NOCRC_OFFSET			16
#define MACB_TX_NOCRC_SIZE			1
#define MACB_MSS_MFS_OFFSET			16
#define MACB_MSS_MFS_SIZE			14
#define MACB_TX_LSO_OFFSET			17
#define MACB_TX_LSO_SIZE			2
#define MACB_TX_TCP_SEQ_SRC_OFFSET		19
#define MACB_TX_TCP_SEQ_SRC_SIZE		1
#define MACB_TX_BUF_EXHAUSTED_OFFSET		27
#define MACB_TX_BUF_EXHAUSTED_SIZE		1
#define MACB_TX_UNDERRUN_OFFSET			28
#define MACB_TX_UNDERRUN_SIZE			1
#define MACB_TX_ERROR_OFFSET			29
#define MACB_TX_ERROR_SIZE			1
#define MACB_TX_WRAP_OFFSET			30
#define MACB_TX_WRAP_SIZE			1
#define MACB_TX_USED_OFFSET			31
#define MACB_TX_USED_SIZE			1

#define GEM_TX_FRMLEN_OFFSET			0
#define GEM_TX_FRMLEN_SIZE			14

/* Buffer descriptor constants */
#define GEM_RX_CSUM_NONE			0
#define GEM_RX_CSUM_IP_ONLY			1
#define GEM_RX_CSUM_IP_TCP			2
#define GEM_RX_CSUM_IP_UDP			3

/* limit RX checksum offload to TCP and UDP packets */
#define GEM_RX_CSUM_CHECKED_MASK		2
#define gem_writel_queue_TBQP(value, queue_num)		\
	write32(ARM_MACB_BASE + GEM_TBQP(queue_num), (value))
#define gem_writel_queue_TBQPH(value, queue_num)	\
	write32(ARM_MACB_BASE + GEM_TBQPH(queue_num), (value))
#define gem_writel_queue_RBQP(value, queue_num)		\
	write32(ARM_MACB_BASE + GEM_RBQP(queue_num), (value))
#define gem_writel_queue_RBQPH(value, queue_num)	\
	write32(ARM_MACB_BASE + GEM_RBQPH(queue_num), (value))


/*
 * These buffer sizes must be power of 2 and divisible
 * by RX_BUFFER_MULTIPLE
 */
#define GEM_RX_BUFFER_SIZE	2048
#define RX_BUFFER_MULTIPLE	64

#define GEM_TX_BUFFER_SIZE	2048

#define MACB_RX_RING_SIZE	64
#define MACB_TX_RING_SIZE	16

#define DMA_DESC_SIZE		16
#define DMA_DESC_BYTES(n)	((n) * DMA_DESC_SIZE)
#define MACB_TX_DMA_DESC_SIZE	(DMA_DESC_BYTES(MACB_TX_RING_SIZE))
#define MACB_RX_DMA_DESC_SIZE	(DMA_DESC_BYTES(MACB_RX_RING_SIZE))
#define MACB_TX_DUMMY_DMA_DESC_SIZE (DMA_DESC_BYTES(1))

#define MACB_TX_TIMEOUT		1000	// us

#define RXBUF_FRMLEN_MASK	0x00000fff
#define TXBUF_FRMLEN_MASK	0x000007ff


#define PHY_ID			0x01    // address of this PHY

#define MII_BMCR                0x00
 #define BMCR_SPEED1000		0x0040	/* MSB of Speed (1000)         */
 #define BMCR_CTST		0x0080	/* Collision test              */
 #define BMCR_FULLDPLX		0x0100	/* Full duplex                 */
 #define BMCR_ANRESTART		0x0200	/* Auto negotiation restart    */
 #define BMCR_ISOLATE		0x0400	/* Isolate data paths from MII */
 #define BMCR_PDOWN		0x0800	/* Enable low power state      */
 #define BMCR_ANENABLE		0x1000	/* Enable auto negotiation     */
 #define BMCR_SPEED100		0x2000	/* Select 100Mbps              */
 #define BMCR_LOOPBACK		0x4000	/* TXD loopback bits           */
 #define BMCR_RESET		0x8000	/* Reset to default state      */
 #define BMCR_SPEED10		0x0000	/* Select 10Mbps               */
#define MII_BMSR                0x01
  #define BMSR_LSTATUS          0x0004
  #define BMSR_ANEGCOMPLETE     0x0020
#define MII_PHYSID1		0x02	/* PHYS ID 1                   */
#define MII_ADVERTISE           0x04
 #define ADVERTISE_CSMA         0x0001
 #define ADVERTISE_10HALF       0x0020
 #define ADVERTISE_1000XFULL    0x0020
 #define ADVERTISE_10FULL       0x0040
 #define ADVERTISE_1000XHALF    0x0040
 #define ADVERTISE_100HALF      0x0080
 #define ADVERTISE_1000XPAUSE   0x0080
 #define ADVERTISE_100FULL      0x0100
 #define ADVERTISE_FULL         (ADVERTISE_100FULL | ADVERTISE_10FULL | ADVERTISE_CSMA)
 #define ADVERTISE_ALL          (ADVERTISE_10HALF | ADVERTISE_10FULL | ADVERTISE_100HALF \
				| ADVERTISE_100FULL)
#define MII_LPA                 0x05
  #define LPA_10HALF            0x0020
  #define LPA_1000XFULL         0x0020
  #define LPA_10FULL            0x0040
  #define LPA_1000XHALF         0x0040
  #define LPA_100HALF           0x0080
  #define LPA_100FULL           0x0100
  #define LPA_100BASE4          0x0200
  #define LPA_PAUSE_CAP         0x0400
#define MII_CTRL1000            0x09
#define MII_STAT1000            0x0A
  #define LPA_1000MSFAIL        0x8000
  #define LPA_1000FULL          0x0800
  #define LPA_1000HALF          0x0400
#define MII_BCM54XX_EXP_DATA    0x15 /* Expansion register data */
#define MII_BCM54XX_EXP_SEL     0x17 /* Expansion register select */
#define MII_BCM54XX_SHD		0x1c /* shadow registers */
 #define MII_BCM54XX_SHD_WRITE	0x8000
 #define MII_BCM54XX_SHD_VAL(x) ((x & 0x1f) << 10)
 #define MII_BCM54XX_SHD_DATA(x) ((x & 0x3ff) << 0)


#define BCM_LED_SRC_MULTICOLOR1 0xa
#define MII_BCM54XX_EXP_SEL_ER  0x0f00 /* Expansion register select */
/* Broadcom Multicolor LED configurations (expansion register 4) */
#define BCM_EXP_MULTICOLOR      (MII_BCM54XX_EXP_SEL_ER + 0x04)
#define BCM_LED_MULTICOLOR_IN_PHASE  (1 << 8)
#define BCM_LED_MULTICOLOR_LINK_ACT  0x0
#define BCM_LED_MULTICOLOR_SPEED     0x1
#define BCM_LED_MULTICOLOR_ACT_FLASH 0x2
#define BCM_LED_MULTICOLOR_FDX       0x3
#define BCM_LED_MULTICOLOR_OFF       0x4
#define BCM_LED_MULTICOLOR_ON        0x5
#define BCM_LED_MULTICOLOR_ALT       0x6
#define BCM_LED_MULTICOLOR_FLASH     0x7
#define BCM_LED_MULTICOLOR_LINK      0x8
#define BCM_LED_MULTICOLOR_ACT       0x9
#define BCM_LED_MULTICOLOR_PROGRAM   0xa
#define BCM54XX_SHD_LEDS1           0x0d /* LED Selector 1 */
#define BCM54XX_SHD_LEDS1_LED3(src) ((src & 0xf) << 4)
#define BCM54XX_SHD_LEDS1_LED1(src) ((src & 0xf) << 0)

#define _1000BASET		1000
#define _100BASET		100
#define _10BASET		10

/**
 * upper_32_bits - return bits 32-63 of a number
 * n: the number we're accessing
 *
 * A basic shift-right of a 64- or 32-bit quantity.  Use this to suppress
 * the "right shift count >= width of type" warning when that quantity is
 * 32-bits.
 */
#define upper_32_bits(n)	((u32)(((n) >> 16) >> 16))

/**
 * lower_32_bits - return bits 0-31 of a number
 * n: the number we're accessing
 */
#define lower_32_bits(n)	((u32)(n))

#define PTR_TO_DMA(p)		((uintptr) (p) | CBcmPCIeHostBridge::GetDMAAddress ())

LOGMODULE ("macb");

CMACBDevice::CMACBDevice (void)
:	m_phy_addr (PHY_ID),
	m_link (0)
{
	m_PHYResetPin.AssignPin (GPIO_PHY_RESET);
	m_PHYResetPin.Write (HIGH);
	m_PHYResetPin.SetMode (GPIOModeOutput, FALSE);
}

CMACBDevice::~CMACBDevice (void)
{
	macb_halt ();

	m_PHYResetPin.SetMode (GPIOModeInput);
}

boolean CMACBDevice::Initialize (void)
{
	// Fetch local MAC address from DTB

	const CDeviceTreeBlob *pDTB = CMachineInfo::Get ()->GetDTB ();
	const TDeviceTreeNode *pEthernet0Node;
	const TDeviceTreeProperty *pLocalMACAddress;

	if (   !pDTB
	    || !(pEthernet0Node = pDTB->FindNode ("/axi/pcie@120000/rp1/ethernet@100000"))
	    || !(pLocalMACAddress = pDTB->FindProperty (pEthernet0Node, "local-mac-address"))
	    || pDTB->GetPropertyValueLength (pLocalMACAddress) != 6)
	{
		LOGERR ("Cannot get MAC address from DTB");

		return FALSE;
	}

	m_MACAddress.Set (pDTB->GetPropertyValue (pLocalMACAddress));

	CString MACString;
	m_MACAddress.Format (&MACString);
	LOGDBG ("MAC address is %s", (const char *) MACString);

	// Check driver prerequisites

	assert (MACB_BFEXT (IDNUM, macb_readl (MID)) >= 2);	// Is GEM controller
	assert (GEM_BFEXT (DAW64, gem_readl (DCFG6)));		// DMA is 64-bit capable

	// Allocate DMA buffers

	int res = eth_alloc ();
	if (res)
	{
		LOGERR ("Cannot allocate DMA buffers (%d)", res);

		return FALSE;
	}

	// Initialize MACB

	write_hwaddr ();

	macb_halt ();

	res = macb_initialize ();
	if (res)
	{
		LOGERR ("Cannot initialize MACB (%d)", res);

		return FALSE;
	}

	// Add network device

	AddNetDevice ();

	return TRUE;
}

const CMACAddress *CMACBDevice::GetMACAddress (void) const
{
	return &m_MACAddress;
}

boolean CMACBDevice::IsSendFrameAdvisable (void)
{
	if (!m_tx_outstanding)
	{
		return TRUE;
	}

	DataSyncBarrier ();
	u32 ctrl = m_tx_ring[m_tx_head].ctrl;
	if (!(ctrl & MACB_BIT (TX_USED)))
	{
		// After a timeout just assume, TX is completed.
		unsigned nTicks = CTimer::Get ()->GetClockTicks ();
		if (   m_tx_last_ticks
		    && nTicks - m_tx_last_ticks > MACB_TX_TIMEOUT * (CLOCKHZ / 1000000))
		{
			// LOGWARN ("TX timeout");

			goto Timeout;
		}

		return FALSE;
	}

	assert (!(ctrl & MACB_BIT (TX_UNDERRUN)));
	assert (!(ctrl & MACB_BIT (TX_BUF_EXHAUSTED)));

Timeout:
	if (++m_tx_head == MACB_TX_RING_SIZE)
	{
		m_tx_head = 0;
	}

	m_tx_outstanding = 0;

	return TRUE;
}

boolean CMACBDevice::SendFrame (const void *pBuffer, unsigned nLength)
{
	assert (pBuffer);
	assert (nLength);
	assert (nLength <= FRAME_BUFFER_SIZE);

	if (!m_link)
	{
		return TRUE;
	}

	if (!IsSendFrameAdvisable ())
	{
		return FALSE;
	}

	m_tx_outstanding = 1;

	assert (m_tx_buffer);
	memcpy (m_tx_buffer, pBuffer, nLength);
	DataMemBarrier ();

	u32 ctrl = nLength & TXBUF_FRMLEN_MASK;
	ctrl |= MACB_BIT (TX_LAST);
	if (m_tx_head == (MACB_TX_RING_SIZE - 1))
	{
		ctrl |= MACB_BIT (TX_WRAP);
	}

	set_addr (&m_tx_ring[m_tx_head], m_tx_buffer_dma);
	DataMemBarrier ();
	m_tx_ring[m_tx_head].ctrl = ctrl;
	DataSyncBarrier ();

	macb_writel (NCR, macb_readl (NCR) | MACB_BIT (TSTART));

	m_tx_last_ticks = CTimer::Get ()->GetClockTicks ();

	return TRUE;
}

boolean CMACBDevice::ReceiveFrame (void *pBuffer, unsigned *pResultLength)
{
	assert (pBuffer);
	assert (pResultLength);

	DataSyncBarrier ();
	u32 addr = m_rx_ring[m_rx_tail].addr;
	if (!(addr & MACB_BIT (RX_USED)))
	{
		return FALSE;
	}

	boolean bResult = FALSE;
	void *rx_buffer;
	unsigned length;

	DataMemBarrier ();
	u32 ctrl = m_rx_ring[m_rx_tail].ctrl;
	const u32 mask = MACB_BIT (RX_SOF) | MACB_BIT (RX_EOF);
	if ((ctrl & mask) != mask)
	{
		goto Return;
	}

	rx_buffer = m_rx_buffer + GEM_RX_BUFFER_SIZE * m_rx_tail;
	length = ctrl & RXBUF_FRMLEN_MASK;
	if (   !length
	    || length > FRAME_BUFFER_SIZE)
	{
		goto Return;
	}

	DataMemBarrier ();
	memcpy (pBuffer, rx_buffer, length);

	*pResultLength = length;

	bResult = TRUE;

Return:
	/* Reclaim RX buffer */
	m_rx_ring[m_rx_tail].ctrl = 0;
	DataMemBarrier ();
	m_rx_ring[m_rx_tail].addr = addr & ~MACB_BIT (RX_USED);
	DataSyncBarrier ();

	if (++m_rx_tail >= MACB_RX_RING_SIZE)
	{
		m_rx_tail = 0;
	}

	return bResult;
}

boolean CMACBDevice::IsLinkUp (void)
{
	UpdatePHY ();

	return !!m_link;
}

TNetDeviceSpeed CMACBDevice::GetLinkSpeed (void)
{
	if (!m_link)
	{
		return NetDeviceSpeedUnknown;
	}

	switch (m_speed)
	{
	case _10BASET:
		return m_duplex ? NetDeviceSpeed10Full : NetDeviceSpeed10Half;

	case _100BASET:
		return m_duplex ? NetDeviceSpeed100Full : NetDeviceSpeed100Half;

	case _1000BASET:
		return m_duplex ? NetDeviceSpeed1000Full : NetDeviceSpeed1000Half;

	default:
		return NetDeviceSpeedUnknown;
	}
}

boolean CMACBDevice::UpdatePHY (void)
{
	int status = 0;
	if (   !m_tx_outstanding
	    && (status = phy_read_status ()) == 1
	    && m_link)
	{
		phy_setup ();
	}

	if (status == 1)
	{
		LOGDBG ("Link is %s", m_link ? "up" : "down");
	}

	return TRUE;
}

int CMACBDevice::eth_alloc (void)
{
	uintptr ulMemStart = CMemorySystem::GetCoherentPage (COHERENT_SLOT_MACB_START);
	uintptr ulMemEnd = CMemorySystem::GetCoherentPage (COHERENT_SLOT_MACB_END + 1);

	m_rx_buffer = reinterpret_cast<u8 *> (ulMemStart);
	m_tx_buffer = m_rx_buffer + GEM_RX_BUFFER_SIZE * MACB_RX_RING_SIZE;

	m_rx_ring = reinterpret_cast<TDMADescriptor *> (m_tx_buffer + GEM_TX_BUFFER_SIZE);
	m_tx_ring = m_rx_ring + MACB_RX_RING_SIZE;
	m_dummy_desc = m_tx_ring + MACB_TX_RING_SIZE;

	if (ulMemEnd < (uintptr) (m_dummy_desc + 1))
	{
		return -1;
	}

	m_rx_buffer_dma = PTR_TO_DMA (m_rx_buffer);
	m_tx_buffer_dma = PTR_TO_DMA (m_tx_buffer);
	m_rx_ring_dma = PTR_TO_DMA (m_rx_ring);
	m_tx_ring_dma = PTR_TO_DMA (m_tx_ring);
	m_dummy_desc_dma = PTR_TO_DMA (m_dummy_desc);

	/*
	 * Do some basic initialization so that we at least can talk
	 * to the PHY
	 */
	u32 ncfgr = gem_mdc_clk_div (0);
	ncfgr |= macb_dbw ();
	ncfgr |= MACB_BIT (DRFCS);		/* Discard Rx FCS */
	macb_writel (NCFGR, ncfgr);

	return 0;
}

int CMACBDevice::macb_initialize (void)
{
	/*
	* macb_halt should have been called at some point before now,
	* so we'll assume the controller is idle.
	*/

	/* initialize DMA descriptors */
	uintptr paddr = m_rx_buffer_dma;
	for (int i = 0; i < MACB_RX_RING_SIZE; i++)
	{
		if (i == (MACB_RX_RING_SIZE - 1))
			paddr |= MACB_BIT(RX_WRAP);
		m_rx_ring[i].ctrl = 0;
		set_addr(&m_rx_ring[i], paddr);
		paddr += GEM_RX_BUFFER_SIZE;
	}

	for (int i = 0; i < MACB_TX_RING_SIZE; i++)
	{
		set_addr(&m_tx_ring[i], 0);
		if (i == (MACB_TX_RING_SIZE - 1))
			m_tx_ring[i].ctrl = MACB_BIT(TX_USED) | MACB_BIT(TX_WRAP);
		else
			m_tx_ring[i].ctrl = MACB_BIT(TX_USED);
	}

	DataSyncBarrier ();

	m_rx_tail = 0;

	m_tx_head = 0;
	m_tx_outstanding = 0;
	m_tx_last_ticks = 0;

	macb_writel(RBQP, lower_32_bits(m_rx_ring_dma));
	macb_writel(TBQP, lower_32_bits(m_tx_ring_dma));
	macb_writel(RBQPH, upper_32_bits(m_rx_ring_dma));
	macb_writel(TBQPH, upper_32_bits(m_tx_ring_dma));

	/* Initialize DMA properties */
	gmac_configure_dma();

	/* Check the multi queue and initialize the queue for TX */
	gmac_init_multi_queues();

	/* Initialize PHY */
	gem_writel(USRIO, GEM_BIT(RGMII));

	int ret = phy_init();
	if (ret)
		return ret;

	/* Enable TX and RX */
	macb_writel(NCR, MACB_BIT(TE) | MACB_BIT(RE));

	return 0;
}

void CMACBDevice::macb_halt (void)
{
	/* Halt the controller and wait for any ongoing transmission to end. */
	u32 ncr = macb_readl(NCR);
	ncr |= MACB_BIT(THALT);
	macb_writel(NCR, ncr);

	u32 tsr;
	do
	{
		tsr = macb_readl(TSR);
	}
	while (tsr & MACB_BIT(TGO));

	/* Disable TX and RX, and clear statistics */
	macb_writel(NCR, MACB_BIT(CLRSTAT));
}

/*
 * Get the DMA bus width field of the network configuration register that we
 * should program. We find the width from decoding the design configuration
 * register to find the maximum supported data bus width.
 */
u32 CMACBDevice::macb_dbw (void)
{
	switch(GEM_BFEXT(DBWDEF, gem_readl(DCFG1)))
	{
		case 4: return GEM_BF(DBW, GEM_DBW128);
		case 2: return GEM_BF(DBW, GEM_DBW64);
		case 1:
		default: return GEM_BF(DBW, GEM_DBW32);
	}
}

u32 CMACBDevice::gem_mdc_clk_div (int id)
{
	unsigned long macb_hz = PCLK_RATE;
	u32 config;

	if (macb_hz < 20000000)
		config = GEM_BF(CLK, GEM_CLK_DIV8);
	else if(macb_hz < 40000000)
		config = GEM_BF(CLK, GEM_CLK_DIV16);
	else if(macb_hz < 80000000)
		config = GEM_BF(CLK, GEM_CLK_DIV32);
	else if(macb_hz < 120000000)
		config = GEM_BF(CLK, GEM_CLK_DIV48);
	else if(macb_hz < 160000000)
		config = GEM_BF(CLK, GEM_CLK_DIV64);
	else if(macb_hz < 240000000)
		config = GEM_BF(CLK, GEM_CLK_DIV96);
	else if(macb_hz < 320000000)
		config = GEM_BF(CLK, GEM_CLK_DIV128);
	else
		config = GEM_BF(CLK, GEM_CLK_DIV224);

	return config;
}

void CMACBDevice::write_hwaddr (void)
{
	const u8 *enetaddr = m_MACAddress.Get ();
	assert (enetaddr);

	/* Set hardware address */
	u32 hwaddr_bottom = enetaddr[0] | enetaddr[1] << 8 | enetaddr[2] << 16 | enetaddr[3] << 24;
	macb_writel(SA1B, hwaddr_bottom);

	u16 hwaddr_top = enetaddr[4] | enetaddr[5] << 8;
	macb_writel(SA1T, hwaddr_top);

	/* Clear unused address register sets */
	macb_writel(SA2B, 0);
	macb_writel(SA2T, 0);
	macb_writel(SA3B, 0);
	macb_writel(SA3T, 0);
	macb_writel(SA4B, 0);
	macb_writel(SA4T, 0);
}

void CMACBDevice::set_addr (TDMADescriptor *desc, uintptr addr)
{
	desc->addrh = upper_32_bits (addr);
	DataMemBarrier ();
	desc->addr = lower_32_bits (addr);
}

void CMACBDevice::gmac_configure_dma (void)
{
	const u32 buffer_size = GEM_RX_BUFFER_SIZE / RX_BUFFER_MULTIPLE;

	u32 dmacfg = gem_readl(DMACFG) & ~GEM_BF(RXBS, -1L);
	dmacfg |= GEM_BF(RXBS, buffer_size);
#if DMA_BURST_LENGTH != 0
	dmacfg = GEM_BFINS(FBLDO, DMA_BURST_LENGTH, dmacfg);
#endif
	dmacfg |= GEM_BIT(TXPBMS) | GEM_BF(RXBMS, -1L);
	dmacfg &= ~GEM_BIT(ENDIA_PKT);
	dmacfg &= ~GEM_BIT(ENDIA_DESC); /* little endian */
	dmacfg |= GEM_BIT(ADDR64);
	gem_writel(DMACFG, dmacfg);
}

void CMACBDevice::gmac_init_multi_queues (void)
{
	int num_queues = 1;

	/* bit 0 is never set but queue 0 always exists */
	u32 queue_mask = gem_readl (DCFG6) & 0xff;
	queue_mask |= 0x1;
	for (int i = 1; i < MACB_MAX_QUEUES; i++)
	{
		if (queue_mask & (1 << i))
			num_queues++;
	}

	m_dummy_desc->ctrl = MACB_BIT (TX_USED);
	m_dummy_desc->addr = 0;

	DataSyncBarrier ();

	uintptr paddr = m_dummy_desc_dma;
	for (int i = 1; i < num_queues; i++)
	{
		gem_writel_queue_TBQP (lower_32_bits (paddr), i - 1);
		gem_writel_queue_RBQP (lower_32_bits (paddr), i - 1);
		gem_writel_queue_TBQPH (upper_32_bits (paddr), i - 1);
		gem_writel_queue_RBQPH (upper_32_bits (paddr), i - 1);
	}
}

int CMACBDevice::phy_init (void)
{
	m_link = 0;
	m_old_link = -1;
	m_old_speed = -1;
	m_old_duplex = -1;

	/* Auto-detect phy_addr */
	int ret = phy_find();
	if (ret)
		return ret;

	/* Check if the PHY is up to snuff... */
	u16 phy_id = mdio_read(m_phy_addr, MII_PHYSID1);
	if (phy_id == 0xffff)
	{
		LOGERR("No PHY present");
		return -1;
	}

	/* Activity leds */
	phy_write_shadow(BCM54XX_SHD_LEDS1,
			   BCM54XX_SHD_LEDS1_LED1(BCM_LED_SRC_MULTICOLOR1)
			 | BCM54XX_SHD_LEDS1_LED3(BCM_LED_SRC_MULTICOLOR1));

	phy_write_exp(BCM_EXP_MULTICOLOR,
		        BCM_LED_MULTICOLOR_IN_PHASE
		      | BCM54XX_SHD_LEDS1_LED1(BCM_LED_MULTICOLOR_LINK_ACT)
		      | BCM54XX_SHD_LEDS1_LED3(BCM_LED_MULTICOLOR_OFF));

	/* Start auto negotiation */
	mdio_write(m_phy_addr, MII_ADVERTISE, ADVERTISE_CSMA | ADVERTISE_ALL);
	mdio_write(m_phy_addr, MII_BMCR, (BMCR_ANENABLE | BMCR_ANRESTART));

	return 0;
}

void CMACBDevice::phy_setup (void)
{
	u32 ncfgr = macb_readl(NCFGR);
	ncfgr &= ~(MACB_BIT(SPD) | MACB_BIT(FD) | GEM_BIT(GBE));

	u16 stat1000 = mdio_read(m_phy_addr, MII_STAT1000);
	if (stat1000 & (LPA_1000FULL | LPA_1000HALF | LPA_1000XFULL | LPA_1000XHALF))
	{
		phy_write_exp(BCM_EXP_MULTICOLOR,
			        BCM_LED_MULTICOLOR_IN_PHASE
			      | BCM54XX_SHD_LEDS1_LED1(BCM_LED_MULTICOLOR_LINK_ACT)
			      | BCM54XX_SHD_LEDS1_LED3(BCM_LED_MULTICOLOR_LINK));

		m_old_speed = m_speed = _1000BASET;
		m_old_duplex = m_duplex = (stat1000 & (LPA_1000FULL | LPA_1000XFULL)) ? 1 : 0;

		ncfgr |= GEM_BIT(GBE);
	}
	else
	{
		phy_write_exp(BCM_EXP_MULTICOLOR,
				BCM_LED_MULTICOLOR_IN_PHASE
			     | BCM54XX_SHD_LEDS1_LED1(BCM_LED_MULTICOLOR_LINK_ACT)
			     | BCM54XX_SHD_LEDS1_LED3(BCM_LED_MULTICOLOR_OFF));

		unsigned media = mii_nway_result(  mdio_read(m_phy_addr, MII_LPA)
						 & mdio_read(m_phy_addr, MII_ADVERTISE));

		if (media & (ADVERTISE_100FULL | ADVERTISE_100HALF))
		{
			m_old_speed = m_speed = _100BASET;

			ncfgr |= MACB_BIT(SPD);
		}
		else
			m_old_speed = m_speed = _10BASET;

		m_old_duplex = m_duplex = (media & ADVERTISE_FULL) ? 1 : 0;
	}

	if (m_duplex)
		ncfgr |= MACB_BIT(FD);

	macb_writel(NCFGR, ncfgr);

	m_old_link = m_link = 1;
}

int CMACBDevice::phy_find (void)
{
	mdio_reset();

	u16 phy_id = mdio_read(m_phy_addr, MII_PHYSID1);
	if (phy_id != 0xffff)
	{
		// LOGDBG ("PHY present at %d", (int) m_phy_addr);
		return 0;
	}

	/* Search for PHY... */
	for (int i = 0; i < 32; i++)
	{
		m_phy_addr = i;
		phy_id = mdio_read(m_phy_addr, MII_PHYSID1);
		if (phy_id != 0xffff)
		{
			// LOGDBG ("PHY present at %d", i);
			return 0;
		}
	}

	LOGERR ("PHY not found");

	return -1;
}

int CMACBDevice::phy_read_status (void)
{
	// Update the link status, return if there was an error
	u16 bmsr = mdio_read (m_phy_addr, MII_BMSR);
	if (!(bmsr & BMSR_LSTATUS))
	{
		m_link = 0;

		if(m_old_link)
		{
			m_old_link = 0;

			return 1;
		}

		return 0;
	}

	// Read autonegotiation status
	u16 lpagb = mdio_read (m_phy_addr, MII_STAT1000);
	u16 ctrl1000 = mdio_read (m_phy_addr, MII_CTRL1000);
	if (lpagb & LPA_1000MSFAIL)
	{
		LOGWARN ("Master/Slave resolution failed (0x%04x)", ctrl1000);

		return -1;
	}

	m_link = 1;

	u16 common_adv_gb = lpagb & ctrl1000 << 2;
	u16 lpa = mdio_read (m_phy_addr, MII_LPA);
	u16 adv = mdio_read (m_phy_addr, MII_ADVERTISE);
	u16 common_adv = lpa & adv;

	m_speed = _10BASET;
	m_duplex = 0;
	if (common_adv_gb & (LPA_1000FULL | LPA_1000HALF))
	{
		m_speed = _1000BASET;
		if (common_adv_gb & LPA_1000FULL)
			m_duplex = 1;
	}
	else if (common_adv & (LPA_100FULL | LPA_100HALF))
	{
		m_speed = _100BASET;
		if (common_adv & LPA_100FULL)
			m_duplex = 1;
	}
	else if (common_adv & LPA_10FULL)
		m_duplex = 1;

	int changed =    (m_link != m_old_link)
		      || (m_speed != m_old_speed)
		      || (m_duplex != m_old_duplex) ? 1 : 0;

	m_old_link = 1;
	m_old_speed = m_speed;
	m_old_duplex = m_duplex;

	return changed;
}

void CMACBDevice::phy_write_shadow (u16 shadow, u16 val)
{
	mdio_write(m_phy_addr, MII_BCM54XX_SHD,   MII_BCM54XX_SHD_WRITE
						| MII_BCM54XX_SHD_VAL(shadow)
						| MII_BCM54XX_SHD_DATA(val));
}

void CMACBDevice::phy_write_exp (u16 reg, u16 val)
{
	mdio_write(m_phy_addr, MII_BCM54XX_EXP_SEL, reg);
	mdio_write(m_phy_addr, MII_BCM54XX_EXP_DATA, val);
}

void CMACBDevice::mdio_write(u8 phy_adr, u8 reg, u16 value)
{
	unsigned long netstat, frame;

	unsigned long netctl = macb_readl(NCR);
	netctl |= MACB_BIT(MPE);
	macb_writel(NCR, netctl);

	frame =   MACB_BF(SOF, 1) | MACB_BF(RW, 1) | MACB_BF(PHYA, phy_adr) | MACB_BF(REGA, reg)
		| MACB_BF(CODE, 2) | MACB_BF(DATA, value);
	macb_writel(MAN, frame);

	do
	{
		netstat = macb_readl(NSR);
	}
	while(!(netstat & MACB_BIT(IDLE)));

	netctl = macb_readl(NCR);
	netctl &= ~MACB_BIT(MPE);
	macb_writel(NCR, netctl);
}

u16 CMACBDevice::mdio_read(u8 phy_adr, u8 reg)
{
	unsigned long netstat, frame;

	unsigned long netctl = macb_readl(NCR);
	netctl |= MACB_BIT(MPE);
	macb_writel(NCR, netctl);

	frame =   MACB_BF(SOF, 1) | MACB_BF(RW, 2) | MACB_BF(PHYA, phy_adr) | MACB_BF(REGA, reg)
		| MACB_BF(CODE, 2);
	macb_writel(MAN, frame);

	do
	{
		netstat = macb_readl(NSR);
	}
	while(!(netstat & MACB_BIT(IDLE)));

	frame = macb_readl(MAN);

	netctl = macb_readl(NCR);
	netctl &= ~MACB_BIT(MPE);
	macb_writel(NCR, netctl);

	return MACB_BFEXT(DATA, frame);
}

void CMACBDevice::mdio_reset(void)
{
	m_PHYResetPin.Write (LOW);	/* phy-reset active low */
	CTimer::Get ()->MsDelay (10);

	m_PHYResetPin.Write (HIGH);
	CTimer::Get ()->MsDelay (10);
}

unsigned CMACBDevice::mii_nway_result (unsigned negotiated)
{
	unsigned ret;
	if (negotiated & LPA_100FULL)
		ret = LPA_100FULL;
	else if(negotiated & LPA_100BASE4)
		ret = LPA_100BASE4;
	else if(negotiated & LPA_100HALF)
		ret = LPA_100HALF;
	else if(negotiated & LPA_10FULL)
		ret = LPA_10FULL;
	else
		ret = LPA_10HALF;

	return ret;
}
