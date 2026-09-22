// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * DRM Driver for DSI output on Raspberry Pi RP1
 *
 * Copyright (c) 2023 Raspberry Pi Limited.
 *
 * Ported to Circle by Rene Stange
 */
#include "rp1dsihostcontroller.h"
#include "linuxcompat.h"
#include <circle/memio.h>
#include <circle/bcm2712.h>
#include <circle/rp1int.h>
#include <circle/machineinfo.h>
#include <circle/bcmpciehostbridge.h>
#include <circle/logger.h>
#include <circle/macros.h>
#include <circle/util.h>
#include <assert.h>

//#define __LOGDBG	LOGDBG
#define __LOGDBG(...)	((void) 0)

/* ------------------------------- Synopsis DSI ------------------------ */
#define DSI_VERSION_CFG			0x000
#define DSI_PWR_UP			0x004
#define DSI_CLKMGR_CFG			0x008
#define DSI_DPI_VCID			0x00C
#define DSI_DPI_COLOR_CODING		0x010
#define DSI_DPI_CFG_POL			0x014
#define DSI_DPI_LP_CMD_TIM		0x018
#define DSI_DBI_VCID			0x01C
#define DSI_DBI_CFG			0x020
#define DSI_DBI_PARTITIONING_EN		0x024
#define DSI_DBI_CMDSIZE			0x028
#define DSI_PCKHDL_CFG			0x02C
 #define DSI_PCKHDL_EOTP_TX_EN		BIT(0)
 #define DSI_PCKHDL_BTA_EN		BIT(2)
#define DSI_GEN_VCID			0x030
#define DSI_MODE_CFG			0x034
#define DSI_VID_MODE_CFG		0x038
 #define DSI_VID_MODE_LP_CMD_EN		BIT(15)
 #define DSI_VID_MODE_FRAME_BTA_ACK_EN	BIT(14)
 #define DSI_VID_MODE_LP_HFP_EN		BIT(13)
 #define DSI_VID_MODE_LP_HBP_EN		BIT(12)
 #define DSI_VID_MODE_LP_VACT_EN	BIT(11)
 #define DSI_VID_MODE_LP_VFP_EN		BIT(10)
 #define DSI_VID_MODE_LP_VBP_EN		BIT(9)
 #define DSI_VID_MODE_LP_VSA_EN		BIT(8)
 #define DSI_VID_MODE_SYNC_PULSES	0
 #define DSI_VID_MODE_SYNC_EVENTS	1
 #define DSI_VID_MODE_BURST		2
#define DSI_VID_PKT_SIZE		0x03C
#define DSI_VID_NUM_CHUNKS		0x040
#define DSI_VID_NULL_SIZE		0x044
#define DSI_VID_HSA_TIME		0x048
#define DSI_VID_HBP_TIME		0x04C
#define DSI_VID_HLINE_TIME		0x050
#define DSI_VID_VSA_LINES		0x054
#define DSI_VID_VBP_LINES		0x058
#define DSI_VID_VFP_LINES		0x05C
#define DSI_VID_VACTIVE_LINES		0x060
#define DSI_EDPI_CMD_SIZE		0x064
#define DSI_CMD_MODE_CFG		0x068
 #define DSI_CMD_MODE_ALL_LP		0x10f7f00
 #define DSI_CMD_MODE_ACK_RQST_EN	BIT(1)
#define DSI_GEN_HDR			0x06C
#define DSI_GEN_PLD_DATA		0x070
#define DSI_CMD_PKT_STATUS		0x074
#define DSI_TO_CNT_CFG			0x078
#define DSI_HS_RD_TO_CNT		0x07C
#define DSI_LP_RD_TO_CNT		0x080
#define DSI_HS_WR_TO_CNT		0x084
#define DSI_LP_WR_TO_CNT		0x088
#define DSI_BTA_TO_CNT			0x08C
#define DSI_SDF_3D			0x090
#define DSI_LPCLK_CTRL			0x094
#define DSI_PHY_TMR_LPCLK_CFG		0x098
 #define DSI_PHY_TMR_HS2LP_LSB		16
 #define DSI_PHY_TMR_LP2HS_LSB		0
#define DSI_PHY_TMR_CFG			0x09C
#define DSI_PHY_TMR_RD_CFG		0x0F4
#define DSI_PHYRSTZ			0x0A0
 #define DSI_PHYRSTZ_SHUTDOWNZ_LSB	0
 #define DSI_PHYRSTZ_SHUTDOWNZ_BITS	BIT(DSI_PHYRSTZ_SHUTDOWNZ_LSB)
 #define DSI_PHYRSTZ_RSTZ_LSB		1
 #define DSI_PHYRSTZ_RSTZ_BITS		BIT(DSI_PHYRSTZ_RSTZ_LSB)
 #define DSI_PHYRSTZ_ENABLECLK_LSB	2
 #define DSI_PHYRSTZ_ENABLECLK_BITS	BIT(DSI_PHYRSTZ_ENABLECLK_LSB)
 #define DSI_PHYRSTZ_FORCEPLL_LSB	3
 #define DSI_PHYRSTZ_FORCEPLL_BITS	BIT(DSI_PHYRSTZ_FORCEPLL_LSB)
#define DSI_PHY_IF_CFG			0x0A4
#define DSI_PHY_ULPS_CTRL		0x0A8
#define DSI_PHY_TX_TRIGGERS		0x0AC
#define DSI_PHY_STATUS			0x0B0

#define DSI_PHY_TST_CTRL0		0x0B4
 #define DPHY_CTRL0_PHY_TESTCLK_LSB	1
 #define DPHY_CTRL0_PHY_TESTCLK_BITS	BIT(DPHY_CTRL0_PHY_TESTCLK_LSB)
 #define DPHY_CTRL0_PHY_TESTCLR_LSB	0
 #define DPHY_CTRL0_PHY_TESTCLR_BITS	BIT(DPHY_CTRL0_PHY_TESTCLR_LSB)
#define DSI_PHY_TST_CTRL1		0x0B8
 #define DPHY_CTRL1_PHY_TESTDIN_LSB	0
 #define DPHY_CTRL1_PHY_TESTDIN_BITS	(0xff << DPHY_CTRL1_PHY_TESTDIN_LSB)
 #define DPHY_CTRL1_PHY_TESTDOUT_LSB	8
 #define DPHY_CTRL1_PHY_TESTDOUT_BITS	(0xff << DPHY_CTRL1_PHY_TESTDOUT_LSB)
 #define DPHY_CTRL1_PHY_TESTEN_LSB	16
 #define DPHY_CTRL1_PHY_TESTEN_BITS	BIT(DPHY_CTRL1_PHY_TESTEN_LSB)
#define DSI_INT_ST0			0x0BC
#define DSI_INT_ST1			0x0C0
#define DSI_INT_MASK0_CFG		0x0C4
#define DSI_INT_MASK1_CFG		0x0C8
#define DSI_PHY_CAL			0x0CC
#define DSI_HEXP_NPKT_CLR		0x104
#define DSI_HEXP_NPKT_SIZE		0x108
#define DSI_VID_SHADOW_CTRL		0x100

#define DSI_DPI_VCID_ACT		0x10C
#define DSI_DPI_COLOR_CODING_ACT	0x110
#define DSI_DPI_LP_CMD_TIM_ACT		0x118
#define DSI_VID_MODE_CFG_ACT		0x138
#define DSI_VID_PKT_SIZE_ACT		0x13C
#define DSI_VID_NUM_CHUNKS_ACT		0x140
#define DSI_VID_NULL_SIZE_ACT		0x144
#define DSI_VID_HSA_TIME_ACT		0x148
#define DSI_VID_HBP_TIME_ACT		0x14C
#define DSI_VID_HLINE_TIME_ACT		0x150
#define DSI_VID_VSA_LINES_ACT		0x154
#define DSI_VID_VBP_LINES_ACT		0x158
#define DSI_VID_VFP_LINES_ACT		0x15C
#define DSI_VID_VACTIVE_LINES_ACT	0x160
#define DSI_SDF_3D_CFG_ACT		0x190

#define DSI_INT_FORCE0			0x0D8
#define DSI_INT_FORCE1			0x0DC

#define DSI_AUTO_ULPS_MODE		0x0E0
#define DSI_AUTO_ULPS_ENTRY_DELAY       0x0E4
#define DSI_AUTO_ULPS_WAKEUP_TIME       0x0E8
#define DSI_EDPI_ADV_FEATURES		0x0EC

#define DSI_DSC_PARAMETER		0x0F0

/* PHY "test and control mode" registers */
#define DPHY_PLL_BIAS_OFFSET		0x10
 #define DPHY_PLL_BIAS_VCO_RANGE_LSB		3
 #define DPHY_PLL_BIAS_USE_PROGRAMMED_VCO_RANGE	BIT(7)
#define DPHY_PLL_CHARGE_PUMP_OFFSET	0x11
#define DPHY_PLL_LPF_OFFSET		0x12
#define DPHY_PLL_INPUT_DIV_OFFSET	0x17
#define DPHY_PLL_LOOP_DIV_OFFSET	0x18
#define DPHY_PLL_DIV_CTRL_OFFSET	0x19
#define DPHY_CLK_PN_SWAP		0x35
#define DPHY_HS_RX_CTRL_LANE0_OFFSET	0x44
#define DPHY_D0_PN_SWAP			0x45
#define DPHY_D1_PN_SWAP			0x55
#define DPHY_D2_PN_SWAP			0x85
#define DPHY_D3_PN_SWAP			0x95

#define DSI_WRITE(reg, val)		write32(m_ulDSIBase + (reg), (val))
#define DSI_READ(reg)			read32(m_ulDSIBase + (reg))


#define RPI_MIPICFG_CLK2FC_OFFSET		0x00000000
#define RPI_MIPICFG_CFG_OFFSET			0x00000004
#define RPI_MIPICFG_TE_OFFSET			0x00000008
#define RPI_MIPICFG_DPHY_MONITOR_OFFSET		0x00000010
#define RPI_MIPICFG_DPHY_CTRL_0_OFFSET		0x00000014
#define RPI_MIPICFG_DPHY_CTRL_1_OFFSET		0x00000018
#define RPI_MIPICFG_DPHY_CTRL_2_OFFSET		0x0000001c
#define RPI_MIPICFG_DPHY_CTRL_3_OFFSET		0x00000020
#define RPI_MIPICFG_DPHY_CTRL_4_OFFSET		0x00000024
#define RPI_MIPICFG_INTR_OFFSET			0x00000028
#define RPI_MIPICFG_INTE_OFFSET			0x0000002c
 #define RPI_MIPICFG_INTE_DSI_DMA_BITS		0x00000002
#define RPI_MIPICFG_INTF_OFFSET			0x00000030
#define RPI_MIPICFG_INTS_OFFSET			0x00000034
#define RPI_MIPICFG_BLOCK_ID_OFFSET		0x00000038
#define RPI_MIPICFG_INSTANCE_ID_OFFSET		0x0000003c
#define RPI_MIPICFG_RSTSEQ_AUTO_OFFSET		0x00000040
#define RPI_MIPICFG_RSTSEQ_PARALLEL_OFFSET	0x00000044
#define RPI_MIPICFG_RSTSEQ_CTRL_OFFSET		0x00000048
#define RPI_MIPICFG_RSTSEQ_TRIG_OFFSET		0x0000004c
#define RPI_MIPICFG_RSTSEQ_DONE_OFFSET		0x00000050
#define RPI_MIPICFG_DFTSS_OFFSET		0x00000054

#define CFG_WRITE(reg, val)			write32(m_ulCFGBase + (reg ## _OFFSET), (val))
#define CFG_READ(reg)				read32(m_ulCFGBase + (reg ## _OFFSET))


// --- DPI DMA REGISTERS (derived from Argon firmware, via RP1 drivers/mipi, with corrections) ---

// Control
#define DPI_DMA_CONTROL				      0x0
#define DPI_DMA_CONTROL_ARM_SHIFT		      0
#define DPI_DMA_CONTROL_ARM_MASK		      BIT(DPI_DMA_CONTROL_ARM_SHIFT)
#define DPI_DMA_CONTROL_ALIGN16_SHIFT		      2
#define DPI_DMA_CONTROL_ALIGN16_MASK		      BIT(DPI_DMA_CONTROL_ALIGN16_SHIFT)
#define DPI_DMA_CONTROL_AUTO_REPEAT_SHIFT	      1
#define DPI_DMA_CONTROL_AUTO_REPEAT_MASK	      BIT(DPI_DMA_CONTROL_AUTO_REPEAT_SHIFT)
#define DPI_DMA_CONTROL_HIGH_WATER_SHIFT	      3
#define DPI_DMA_CONTROL_HIGH_WATER_MASK		      (0x1FF << DPI_DMA_CONTROL_HIGH_WATER_SHIFT)
#define DPI_DMA_CONTROL_DEN_POL_SHIFT		      12
#define DPI_DMA_CONTROL_DEN_POL_MASK		      BIT(DPI_DMA_CONTROL_DEN_POL_SHIFT)
#define DPI_DMA_CONTROL_HSYNC_POL_SHIFT		      13
#define DPI_DMA_CONTROL_HSYNC_POL_MASK		      BIT(DPI_DMA_CONTROL_HSYNC_POL_SHIFT)
#define DPI_DMA_CONTROL_VSYNC_POL_SHIFT		      14
#define DPI_DMA_CONTROL_VSYNC_POL_MASK		      BIT(DPI_DMA_CONTROL_VSYNC_POL_SHIFT)
#define DPI_DMA_CONTROL_COLORM_SHIFT		      15
#define DPI_DMA_CONTROL_COLORM_MASK		      BIT(DPI_DMA_CONTROL_COLORM_SHIFT)
#define DPI_DMA_CONTROL_SHUTDN_SHIFT		      16
#define DPI_DMA_CONTROL_SHUTDN_MASK		      BIT(DPI_DMA_CONTROL_SHUTDN_SHIFT)
#define DPI_DMA_CONTROL_HBP_EN_SHIFT		      17
#define DPI_DMA_CONTROL_HBP_EN_MASK		      BIT(DPI_DMA_CONTROL_HBP_EN_SHIFT)
#define DPI_DMA_CONTROL_HFP_EN_SHIFT		      18
#define DPI_DMA_CONTROL_HFP_EN_MASK		      BIT(DPI_DMA_CONTROL_HFP_EN_SHIFT)
#define DPI_DMA_CONTROL_VBP_EN_SHIFT		      19
#define DPI_DMA_CONTROL_VBP_EN_MASK		      BIT(DPI_DMA_CONTROL_VBP_EN_SHIFT)
#define DPI_DMA_CONTROL_VFP_EN_SHIFT		      20
#define DPI_DMA_CONTROL_VFP_EN_MASK		      BIT(DPI_DMA_CONTROL_VFP_EN_SHIFT)
#define DPI_DMA_CONTROL_HSYNC_EN_SHIFT		      21
#define DPI_DMA_CONTROL_HSYNC_EN_MASK		      BIT(DPI_DMA_CONTROL_HSYNC_EN_SHIFT)
#define DPI_DMA_CONTROL_VSYNC_EN_SHIFT		      22
#define DPI_DMA_CONTROL_VSYNC_EN_MASK		      BIT(DPI_DMA_CONTROL_VSYNC_EN_SHIFT)
#define DPI_DMA_CONTROL_FORCE_IMMED_SHIFT	      23
#define DPI_DMA_CONTROL_FORCE_IMMED_MASK	      BIT(DPI_DMA_CONTROL_FORCE_IMMED_SHIFT)
#define DPI_DMA_CONTROL_FORCE_DRAIN_SHIFT	      24
#define DPI_DMA_CONTROL_FORCE_DRAIN_MASK	      BIT(DPI_DMA_CONTROL_FORCE_DRAIN_SHIFT)
#define DPI_DMA_CONTROL_FORCE_EMPTY_SHIFT	      25
#define DPI_DMA_CONTROL_FORCE_EMPTY_MASK	      BIT(DPI_DMA_CONTROL_FORCE_EMPTY_SHIFT)

// IRQ_ENABLES
#define DPI_DMA_IRQ_EN				      0x04
#define DPI_DMA_IRQ_EN_DMA_READY_SHIFT		      0
#define DPI_DMA_IRQ_EN_DMA_READY_MASK		      BIT(DPI_DMA_IRQ_EN_DMA_READY_SHIFT)
#define DPI_DMA_IRQ_EN_UNDERFLOW_SHIFT		      1
#define DPI_DMA_IRQ_EN_UNDERFLOW_MASK		      BIT(DPI_DMA_IRQ_EN_UNDERFLOW_SHIFT)
#define DPI_DMA_IRQ_EN_FRAME_START_SHIFT	      2
#define DPI_DMA_IRQ_EN_FRAME_START_MASK		      BIT(DPI_DMA_IRQ_EN_FRAME_START_SHIFT)
#define DPI_DMA_IRQ_EN_AFIFO_EMPTY_SHIFT	      3
#define DPI_DMA_IRQ_EN_AFIFO_EMPTY_MASK		      BIT(DPI_DMA_IRQ_EN_AFIFO_EMPTY_SHIFT)
#define DPI_DMA_IRQ_EN_TE_SHIFT			      4
#define DPI_DMA_IRQ_EN_TE_MASK			      BIT(DPI_DMA_IRQ_EN_TE_SHIFT)
#define DPI_DMA_IRQ_EN_ERROR_SHIFT		      5
#define DPI_DMA_IRQ_EN_ERROR_MASK		      BIT(DPI_DMA_IRQ_EN_ERROR_SHIFT)
#define DPI_DMA_IRQ_EN_MATCH_SHIFT		      6
#define DPI_DMA_IRQ_EN_MATCH_MASK		      BIT(DPI_DMA_IRQ_EN_MATCH_SHIFT)
#define DPI_DMA_IRQ_EN_MATCH_LINE_SHIFT		      16
#define DPI_DMA_IRQ_EN_MATCH_LINE_MASK		      (0xFFF << DPI_DMA_IRQ_EN_MATCH_LINE_SHIFT)

// IRQ_FLAGS
#define DPI_DMA_IRQ_FLAGS			      0x08
#define DPI_DMA_IRQ_FLAGS_DMA_READY_SHIFT	      0
#define DPI_DMA_IRQ_FLAGS_DMA_READY_MASK	      BIT(DPI_DMA_IRQ_FLAGS_DMA_READY_SHIFT)
#define DPI_DMA_IRQ_FLAGS_UNDERFLOW_SHIFT	      1
#define DPI_DMA_IRQ_FLAGS_UNDERFLOW_MASK	      BIT(DPI_DMA_IRQ_FLAGS_UNDERFLOW_SHIFT)
#define DPI_DMA_IRQ_FLAGS_FRAME_START_SHIFT	      2
#define DPI_DMA_IRQ_FLAGS_FRAME_START_MASK	      BIT(DPI_DMA_IRQ_FLAGS_FRAME_START_SHIFT)
#define DPI_DMA_IRQ_FLAGS_AFIFO_EMPTY_SHIFT	      3
#define DPI_DMA_IRQ_FLAGS_AFIFO_EMPTY_MASK	      BIT(DPI_DMA_IRQ_FLAGS_AFIFO_EMPTY_SHIFT)
#define DPI_DMA_IRQ_FLAGS_TE_SHIFT		      4
#define DPI_DMA_IRQ_FLAGS_TE_MASK		      BIT(DPI_DMA_IRQ_FLAGS_TE_SHIFT)
#define DPI_DMA_IRQ_FLAGS_ERROR_SHIFT		      5
#define DPI_DMA_IRQ_FLAGS_ERROR_MASK		      BIT(DPI_DMA_IRQ_FLAGS_ERROR_SHIFT)
#define DPI_DMA_IRQ_FLAGS_MATCH_SHIFT		      6
#define DPI_DMA_IRQ_FLAGS_MATCH_MASK		      BIT(DPI_DMA_IRQ_FLAGS_MATCH_SHIFT)

// QOS
#define DPI_DMA_QOS				      0xC
#define DPI_DMA_QOS_DQOS_SHIFT			      0
#define DPI_DMA_QOS_DQOS_MASK			      (0xF << DPI_DMA_QOS_DQOS_SHIFT)
#define DPI_DMA_QOS_ULEV_SHIFT			      4
#define DPI_DMA_QOS_ULEV_MASK			      (0xF << DPI_DMA_QOS_ULEV_SHIFT)
#define DPI_DMA_QOS_UQOS_SHIFT			      8
#define DPI_DMA_QOS_UQOS_MASK			      (0xF << DPI_DMA_QOS_UQOS_SHIFT)
#define DPI_DMA_QOS_LLEV_SHIFT			      12
#define DPI_DMA_QOS_LLEV_MASK			      (0xF << DPI_DMA_QOS_LLEV_SHIFT)
#define DPI_DMA_QOS_LQOS_SHIFT			      16
#define DPI_DMA_QOS_LQOS_MASK			      (0xF << DPI_DMA_QOS_LQOS_SHIFT)

// Panics
#define DPI_DMA_PANICS				     0x38
#define DPI_DMA_PANICS_UPPER_COUNT_SHIFT	     0
#define DPI_DMA_PANICS_UPPER_COUNT_MASK		     \
				(0x0000FFFF << DPI_DMA_PANICS_UPPER_COUNT_SHIFT)
#define DPI_DMA_PANICS_LOWER_COUNT_SHIFT	     16
#define DPI_DMA_PANICS_LOWER_COUNT_MASK		     \
				(0x0000FFFF << DPI_DMA_PANICS_LOWER_COUNT_SHIFT)

// DMA Address Lower:
#define DPI_DMA_DMA_ADDR_L			     0x10

// DMA Address Upper:
#define DPI_DMA_DMA_ADDR_H			     0x40

// DMA stride
#define DPI_DMA_DMA_STRIDE			     0x14

// Visible Area
#define DPI_DMA_VISIBLE_AREA			     0x18
#define DPI_DMA_VISIBLE_AREA_ROWSM1_SHIFT     0
#define DPI_DMA_VISIBLE_AREA_ROWSM1_MASK     (0x0FFF << DPI_DMA_VISIBLE_AREA_ROWSM1_SHIFT)
#define DPI_DMA_VISIBLE_AREA_COLSM1_SHIFT    16
#define DPI_DMA_VISIBLE_AREA_COLSM1_MASK     (0x0FFF << DPI_DMA_VISIBLE_AREA_COLSM1_SHIFT)

// Sync width
#define DPI_DMA_SYNC_WIDTH   0x1C
#define DPI_DMA_SYNC_WIDTH_ROWSM1_SHIFT	 0
#define DPI_DMA_SYNC_WIDTH_ROWSM1_MASK	 (0x0FFF << DPI_DMA_SYNC_WIDTH_ROWSM1_SHIFT)
#define DPI_DMA_SYNC_WIDTH_COLSM1_SHIFT	 16
#define DPI_DMA_SYNC_WIDTH_COLSM1_MASK	 (0x0FFF << DPI_DMA_SYNC_WIDTH_COLSM1_SHIFT)

// Back porch
#define DPI_DMA_BACK_PORCH   0x20
#define DPI_DMA_BACK_PORCH_ROWSM1_SHIFT	 0
#define DPI_DMA_BACK_PORCH_ROWSM1_MASK	 (0x0FFF << DPI_DMA_BACK_PORCH_ROWSM1_SHIFT)
#define DPI_DMA_BACK_PORCH_COLSM1_SHIFT	 16
#define DPI_DMA_BACK_PORCH_COLSM1_MASK	 (0x0FFF << DPI_DMA_BACK_PORCH_COLSM1_SHIFT)

// Front porch
#define DPI_DMA_FRONT_PORCH  0x24
#define DPI_DMA_FRONT_PORCH_ROWSM1_SHIFT     0
#define DPI_DMA_FRONT_PORCH_ROWSM1_MASK	 (0x0FFF << DPI_DMA_FRONT_PORCH_ROWSM1_SHIFT)
#define DPI_DMA_FRONT_PORCH_COLSM1_SHIFT     16
#define DPI_DMA_FRONT_PORCH_COLSM1_MASK	 (0x0FFF << DPI_DMA_FRONT_PORCH_COLSM1_SHIFT)

// Input masks
#define DPI_DMA_IMASK	 0x2C
#define DPI_DMA_IMASK_R_SHIFT	 0
#define DPI_DMA_IMASK_R_MASK	 (0x3FF << DPI_DMA_IMASK_R_SHIFT)
#define DPI_DMA_IMASK_G_SHIFT	 10
#define DPI_DMA_IMASK_G_MASK	 (0x3FF << DPI_DMA_IMASK_G_SHIFT)
#define DPI_DMA_IMASK_B_SHIFT	 20
#define DPI_DMA_IMASK_B_MASK	 (0x3FF << DPI_DMA_IMASK_B_SHIFT)

// Output Masks
#define DPI_DMA_OMASK	 0x30
#define DPI_DMA_OMASK_R_SHIFT	 0
#define DPI_DMA_OMASK_R_MASK	 (0x3FF << DPI_DMA_OMASK_R_SHIFT)
#define DPI_DMA_OMASK_G_SHIFT	 10
#define DPI_DMA_OMASK_G_MASK	 (0x3FF << DPI_DMA_OMASK_G_SHIFT)
#define DPI_DMA_OMASK_B_SHIFT	 20
#define DPI_DMA_OMASK_B_MASK	 (0x3FF << DPI_DMA_OMASK_B_SHIFT)

// Shifts
#define DPI_DMA_SHIFT	 0x28
#define DPI_DMA_SHIFT_IR_SHIFT	 0
#define DPI_DMA_SHIFT_IR_MASK	 (0x1F << DPI_DMA_SHIFT_IR_SHIFT)
#define DPI_DMA_SHIFT_IG_SHIFT	 5
#define DPI_DMA_SHIFT_IG_MASK	 (0x1F << DPI_DMA_SHIFT_IG_SHIFT)
#define DPI_DMA_SHIFT_IB_SHIFT	 10
#define DPI_DMA_SHIFT_IB_MASK	 (0x1F << DPI_DMA_SHIFT_IB_SHIFT)
#define DPI_DMA_SHIFT_OR_SHIFT	 15
#define DPI_DMA_SHIFT_OR_MASK	 (0x1F << DPI_DMA_SHIFT_OR_SHIFT)
#define DPI_DMA_SHIFT_OG_SHIFT	 20
#define DPI_DMA_SHIFT_OG_MASK	 (0x1F << DPI_DMA_SHIFT_OG_SHIFT)
#define DPI_DMA_SHIFT_OB_SHIFT	 25
#define DPI_DMA_SHIFT_OB_MASK	 (0x1F << DPI_DMA_SHIFT_OB_SHIFT)

// Scaling
#define DPI_DMA_RGBSZ	 0x34
#define DPI_DMA_RGBSZ_BPP_SHIFT	 16
#define DPI_DMA_RGBSZ_BPP_MASK	 (0x3 << DPI_DMA_RGBSZ_BPP_SHIFT)
#define DPI_DMA_RGBSZ_R_SHIFT	 0
#define DPI_DMA_RGBSZ_R_MASK	 (0xF << DPI_DMA_RGBSZ_R_SHIFT)
#define DPI_DMA_RGBSZ_G_SHIFT	 4
#define DPI_DMA_RGBSZ_G_MASK	 (0xF << DPI_DMA_RGBSZ_G_SHIFT)
#define DPI_DMA_RGBSZ_B_SHIFT	 8
#define DPI_DMA_RGBSZ_B_MASK	 (0xF << DPI_DMA_RGBSZ_B_SHIFT)

// Status
#define DPI_DMA_STATUS  0x3c

#define BITS(field, val) (((val) << (field ## _SHIFT)) & (field ## _MASK))

#define PTR_TO_DMA(ptr)		(  reinterpret_cast<uintptr> (ptr)	\
				 | CBcmPCIeHostBridge::GetDMAAddress (PCIE_BUS_RP1))

/* See Table A-3 on page 258 of dphy databook */
const CRP1DSIHostController::hsfreq_range CRP1DSIHostController::s_hsfreq_table[] =
{
	{   89, 0b000000, 32, 20, 26, 13 },
	{   99, 0b010000, 35, 23, 28, 14 },
	{  109, 0b100000, 32, 22, 26, 13 },
	{  129, 0b000001, 31, 20, 27, 13 },
	{  139, 0b010001, 33, 22, 26, 14 },
	{  149, 0b100001, 33, 21, 26, 14 },
	{  169, 0b000010, 32, 20, 27, 13 },
	{  179, 0b010010, 36, 23, 30, 15 },
	{  199, 0b100010, 40, 22, 33, 15 },
	{  219, 0b000011, 40, 22, 33, 15 },
	{  239, 0b010011, 44, 24, 36, 16 },
	{  249, 0b100011, 48, 24, 38, 17 },
	{  269, 0b000100, 48, 24, 38, 17 },
	{  299, 0b010100, 50, 27, 41, 18 },
	{  329, 0b000101, 56, 28, 45, 18 },
	{  359, 0b010101, 59, 28, 48, 19 },
	{  399, 0b100101, 61, 30, 50, 20 },
	{  449, 0b000110, 67, 31, 55, 21 },
	{  499, 0b010110, 73, 31, 59, 22 },
	{  549, 0b000111, 79, 36, 63, 24 },
	{  599, 0b010111, 83, 37, 68, 25 },
	{  649, 0b001000, 90, 38, 73, 27 },
	{  699, 0b011000, 95, 40, 77, 28 },
	{  749, 0b001001, 102, 40, 84, 28 },
	{  799, 0b011001, 106, 42, 87, 30 },
	{  849, 0b101001, 113, 44, 93, 31 },
	{  899, 0b111001, 118, 47, 98, 32 },
	{  949, 0b001010, 124, 47, 102, 34 },
	{  999, 0b011010, 130, 49, 107, 35 },
	{ 1049, 0b101010, 135, 51, 111, 37 },
	{ 1099, 0b111010, 139, 51, 114, 38 },
	{ 1149, 0b001011, 146, 54, 120, 40 },
	{ 1199, 0b011011, 153, 57, 125, 41 },
	{ 1249, 0b101011, 158, 58, 130, 42 },
	{ 1299, 0b111011, 163, 58, 135, 44 },
	{ 1349, 0b001100, 168, 60, 140, 45 },
	{ 1399, 0b011100, 172, 64, 144, 47 },
	{ 1449, 0b101100, 176, 65, 148, 48 },
	{ 1500, 0b111100, 181, 66, 153, 50 },
};

/* Table of supported input (in-memory/DMA) pixel formats. */
struct rp1dsi_ipixfmt
{
	u32 format; /* DRM format code                           */
	u32 mask;   /* RGB masks (10 bits each, left justified)  */
	u32 shift;  /* RGB MSB positions in the memory word      */
	u32 rgbsz;  /* Shifts used for scaling; also (BPP/8-1)   */
};

enum
{
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_XBGR8888,
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_ABGR8888,
	DRM_FORMAT_RGB888,
	DRM_FORMAT_BGR888,
	DRM_FORMAT_RGB565,
};

#define IMASK_RGB(r, g, b)	(BITS(DPI_DMA_IMASK_R, r) | \
				 BITS(DPI_DMA_IMASK_G, g) |  \
				 BITS(DPI_DMA_IMASK_B, b))
#define ISHIFT_RGB(r, g, b)	(BITS(DPI_DMA_SHIFT_IR, r) | \
				 BITS(DPI_DMA_SHIFT_IG, g) | \
				 BITS(DPI_DMA_SHIFT_IB, b))

static const struct rp1dsi_ipixfmt my_formats[] =
{
	{
		.format = DRM_FORMAT_XRGB8888,
		.mask   = IMASK_RGB(0x3fc, 0x3fc, 0x3fc),
		.shift  = ISHIFT_RGB(23, 15, 7),
		.rgbsz  = BITS(DPI_DMA_RGBSZ_BPP, 3),
	},
	{
		.format = DRM_FORMAT_XBGR8888,
		.mask   = IMASK_RGB(0x3fc, 0x3fc, 0x3fc),
		.shift  = ISHIFT_RGB(7, 15, 23),
		.rgbsz  = BITS(DPI_DMA_RGBSZ_BPP, 3),
	},
	{
		.format = DRM_FORMAT_ARGB8888,
		.mask   = IMASK_RGB(0x3fc, 0x3fc, 0x3fc),
		.shift  = ISHIFT_RGB(23, 15, 7),
		.rgbsz  = BITS(DPI_DMA_RGBSZ_BPP, 3),
	},
	{
		.format = DRM_FORMAT_ABGR8888,
		.mask   = IMASK_RGB(0x3fc, 0x3fc, 0x3fc),
		.shift  = ISHIFT_RGB(7, 15, 23),
		.rgbsz  = BITS(DPI_DMA_RGBSZ_BPP, 3),
	},
	{
		.format = DRM_FORMAT_RGB888,
		.mask   = IMASK_RGB(0x3fc, 0x3fc, 0x3fc),
		.shift  = ISHIFT_RGB(23, 15, 7),
		.rgbsz  = BITS(DPI_DMA_RGBSZ_BPP, 2),
	},
	{
		.format = DRM_FORMAT_BGR888,
		.mask   = IMASK_RGB(0x3fc, 0x3fc, 0x3fc),
		.shift  = ISHIFT_RGB(7, 15, 23),
		.rgbsz  = BITS(DPI_DMA_RGBSZ_BPP, 2),
	},
	{
		.format = DRM_FORMAT_RGB565,
		.mask   = IMASK_RGB(0x3e0, 0x3f0, 0x3e0),
		.shift  = ISHIFT_RGB(15, 10, 4),
		.rgbsz  = BITS(DPI_DMA_RGBSZ_R, 5) | BITS(DPI_DMA_RGBSZ_G, 6) |
			  BITS(DPI_DMA_RGBSZ_B, 5) | BITS(DPI_DMA_RGBSZ_BPP, 1),
	}
};

mipi_dsi_host_ops CRP1DSIHostController::s_host_ops =
{
	.transfer = rp1dsi_host_transfer
};

LOGMODULE ("rp1dsi");

CRP1DSIHostController::CRP1DSIHostController (CInterruptSystem *pInterrupt,
					      unsigned nDepth, unsigned nDisplay,
					      unsigned nDataLanes)
:	mipi_dsi_host {&s_host_ops},
	m_pInterrupt (pInterrupt),
	m_nDepth (nDepth),
	m_nDisplay (nDisplay),
	m_nDataLanes (nDataLanes),
	m_pMode (nullptr),
	m_ulDMABase (nDisplay ? ARM_MIPI1_DMA_BASE : ARM_MIPI0_DMA_BASE),
	m_ulDSIBase (nDisplay ? ARM_MIPI1_DSI_BASE : ARM_MIPI0_DSI_BASE),
	m_ulCFGBase (nDisplay ? ARM_MIPI1_CFG_BASE : ARM_MIPI0_CFG_BASE),
	m_CFGClock (nDisplay ? GPIOClockMIPI1CFG : GPIOClockMIPI0CFG),
	/*
	 * Prefer the DSI byte-clock source where possible, so that DSI and DPI
	 * clocks will be in an exact ratio and downstream devices can recover
	 * perfect timings. But when DPI clock is faster, fall back on PLL_SYS.
	 * To defeat rounding errors, specify explicitly which source to use.
	 */
	m_DPIClock (nDisplay ? GPIOClockMIPI1DPI : GPIOClockMIPI0DPI,
		      (nDepth == 16 ? 16 : 24) >= 8 * nDataLanes
		    ? GPIOClockSourceMIPIDSIByteClock
		    : GPIOClockSourcePLLSys),
	m_display_flags (  MIPI_DSI_MODE_VIDEO
			 | MIPI_DSI_MODE_VIDEO_SYNC_PULSE
			 | MIPI_DSI_MODE_LPM
			 | MIPI_DSI_MODE_VIDEO_HSE),
	m_display_format (MIPI_DSI_FMT_RGB888),
	m_bIRQConnected (FALSE),
	m_bDSIRunning (FALSE),
	m_bDMARunning (FALSE),
	m_pVBlankHandler (nullptr)
{
	assert (m_nDepth == 16 || m_nDepth == 32);
	assert (m_nDataLanes == 1 || m_nDataLanes == 2);
}

CRP1DSIHostController::~CRP1DSIHostController (void)
{
	if (m_bDMARunning)
	{
		rp1dsi_dma_stop ();
	}

	if (m_bDSIRunning)
	{
		rp1dsi_dsi_stop ();
	}

	if (m_bIRQConnected)
	{
		assert (m_pInterrupt);
		m_pInterrupt->DisconnectIRQ (m_nDisplay ? RP1_IRQ_MIPI1 : RP1_IRQ_MIPI0);
	}

	m_CFGClock.Stop ();
}

boolean CRP1DSIHostController::Initialize (const drm_display_mode *pMode)
{
	assert (!m_pMode);
	m_pMode = pMode;
	assert (m_pMode);

	assert (!m_bDSIRunning);

	if (!m_CFGClock.StartRate (25000000))
	{
		LOGERR ("Cannot start CFG clock");

		return FALSE;
	}

	assert (!m_bIRQConnected);
	assert (m_pInterrupt);
	m_pInterrupt->ConnectIRQ (m_nDisplay ? RP1_IRQ_MIPI1 : RP1_IRQ_MIPI0, rp1dsi_dma_isr, this);
	m_bIRQConnected = TRUE;

	rp1dsi_mipicfg_setup ();

	int ret = rp1dsi_dsi_setup (m_pMode);
	if (ret < 0)
	{
		LOGERR ("Cannot setup DSI (%d)", ret);

		return FALSE;
	}

	m_bDSIRunning = TRUE;

	return TRUE;
}

boolean CRP1DSIHostController::Start (void *pFrameBuffer, unsigned nPitch)
{
	assert (!m_bDMARunning);

	rp1dsi_dsi_set_cmdmode (0);

	assert (m_nDepth == 16 || m_nDepth == 32);
	rp1dsi_dma_setup (m_nDepth == 16 ? DRM_FORMAT_RGB565 : DRM_FORMAT_ARGB8888,
			  m_display_format, m_pMode);

	rp1dsi_dma_update (PTR_TO_DMA (pFrameBuffer), 0, nPitch);

	m_bDMARunning = TRUE;

	return TRUE;
}

void CRP1DSIHostController::RegisterVBlankHandler (TVBlankHandler *pHandler, void *pParam)
{
	m_pVBlankParam = pParam;
	m_pVBlankHandler = pHandler;
}

ssize_t CRP1DSIHostController::rp1dsi_host_transfer (mipi_dsi_host *host, const mipi_dsi_msg *msg)
{
	CRP1DSIHostController *pHost = static_cast<CRP1DSIHostController *> (host);
	assert (pHost);

	/* Write */
	struct mipi_dsi_packet packet;
	int ret = mipi_dsi_create_packet(&packet, msg);
	if (ret) {
		LOGERR ("Failed to create packet (%d)", ret);
		return ret;
	}

	pHost->rp1dsi_dsi_send(*(u32 *)(&packet.header),
			packet.payload_length, packet.payload,
			!!(msg->flags & MIPI_DSI_MSG_USE_LPM),
			!!(msg->flags & MIPI_DSI_MSG_REQ_ACK));

	/* Optional read back */
	if (msg->rx_len && msg->rx_buf)
		ret = pHost->rp1dsi_dsi_recv(msg->rx_len, (u8 *) msg->rx_buf);

	return (ssize_t)ret;
}

/* ------------------------------- DPHY setup stuff ------------------------ */

void CRP1DSIHostController::dphy_transaction (u8 test_code, u8 test_data)
{
	/*
	 * See pg 101 of mipi dphy bidir databook
	 * Assume we start with testclk high.
	 * Each APB write takes at least 10ns and we ignore TESTDOUT
	 * so there is no need for extra delays between the transitions.
	 */

	DSI_WRITE(DSI_PHY_TST_CTRL1, test_code | DPHY_CTRL1_PHY_TESTEN_BITS);
	DSI_WRITE(DSI_PHY_TST_CTRL0, 0);
	DSI_READ(DSI_PHY_TST_CTRL1); /* XXX possibly not needed */
	DSI_WRITE(DSI_PHY_TST_CTRL1, test_data);
	DSI_WRITE(DSI_PHY_TST_CTRL0, DPHY_CTRL0_PHY_TESTCLK_BITS);
}

u64 CRP1DSIHostController::dphy_get_div (u32 refclk, u64 vco_freq, u32 *ptr_m, u32 *ptr_n)
{
	/*
	 * See pg 77-78 of dphy databook
	 * fvco = m/n * refclk
	 * with the limit
	 * 40MHz >= fREFCLK / N >= 5MHz
	 * M (multiplier) must be an even number between 2 and 300
	 * N (input divider) must be an integer between 1 and 100
	 *
	 * In practice, given a 50MHz reference clock, it can produce any
	 * multiple of 10MHz, 11.1111MHz, 12.5MHz, 14.286MHz or 16.667MHz
	 * with < 1% error for all frequencies above 495MHz.
	 *
	 * vco_freq should be set to the lane bit rate (not the MIPI clock
	 * which is half of this). These frequencies are now measured in Hz.
	 * They should fit within u32, but u64 is needed for calculations.
	 */

	static const u32 REF_DIVN_MAX = 40000000;
	static const u32 REF_DIVN_MIN =  5000000;
	u32 n, best_n = 0, best_m = 0;
	u64 best_err = vco_freq;

	for (n = 1 + refclk / REF_DIVN_MAX; n * REF_DIVN_MIN <= refclk && n < 100; ++n) {
		u32 half_m = DIV_U64_ROUND_CLOSEST(n * vco_freq, 2 * refclk);

		if (half_m < 150) {
			u64 f = div_u64(mul_u32_u32(2 * half_m, refclk), n);
			u64 err = (f > vco_freq) ? f - vco_freq : vco_freq - f;

			if (err < best_err) {
				best_n = n;
				best_m = 2 * half_m;
				best_err = err;
				if (err == 0)
					break;
			}
		}
	}

	if (64 * best_err >= vco_freq)
		return 0;

	*ptr_n = best_n;
	*ptr_m = best_m;
	return div_u64(mul_u32_u32(best_m, refclk), best_n);
}

void CRP1DSIHostController::dphy_set_hsfreqrange (u32 freq_mhz)
{
	unsigned int i;

	if (freq_mhz < 80 || freq_mhz > 1500)
		LOGERR ("DPHY: Frequency %u MHz out of range", freq_mhz);

	for (i = 0; i < ARRAY_SIZE(s_hsfreq_table) - 1; i++) {
		if (freq_mhz <= s_hsfreq_table[i].mhz_max)
			break;
	}

	m_hsfreq_index = i;
	dphy_transaction(DPHY_HS_RX_CTRL_LANE0_OFFSET,
			 s_hsfreq_table[i].hsfreqrange << 1);
}

u32 CRP1DSIHostController::dphy_configure_pll (u32 refclk, u32 vco_freq)
{
	u32 m = 0;
	u32 n = 0;
	u32 actual_vco_freq = dphy_get_div(refclk, vco_freq, &m, &n);

	if (actual_vco_freq) {
		dphy_set_hsfreqrange(actual_vco_freq / 1000000);
		/* Program m,n from registers */
		dphy_transaction(DPHY_PLL_DIV_CTRL_OFFSET, 0x30);
		/* N (program N-1) */
		dphy_transaction(DPHY_PLL_INPUT_DIV_OFFSET, n - 1);
		/* M[8:5] ?? */
		dphy_transaction(DPHY_PLL_LOOP_DIV_OFFSET, 0x80 | ((m - 1) >> 5));
		/* M[4:0] (program M-1) */
		dphy_transaction(DPHY_PLL_LOOP_DIV_OFFSET, ((m - 1) & 0x1F));
		__LOGDBG ("DPHY: vco freq want %uHz got %uHz = %d * (%uHz / %d)",
			  vco_freq, actual_vco_freq, m, refclk, n);
		__LOGDBG ("DPHY: hsfreqrange = 0x%02x",
			  s_hsfreq_table[m_hsfreq_index].hsfreqrange);
	} else {
		LOGERR ("Error configuring DPHY PLL %uHz", vco_freq);
	}

	return actual_vco_freq;
}

u32 CRP1DSIHostController::dphy_init (u32 ref_freq, u32 vco_freq)
{
	u32 actual_vco_freq;

	/* Reset the PHY */
	DSI_WRITE(DSI_PHYRSTZ, 0);
	DSI_WRITE(DSI_PHY_TST_CTRL0, DPHY_CTRL0_PHY_TESTCLK_BITS);
	DSI_WRITE(DSI_PHY_TST_CTRL1, 0);
	DSI_WRITE(DSI_PHY_TST_CTRL0, (DPHY_CTRL0_PHY_TESTCLK_BITS | DPHY_CTRL0_PHY_TESTCLR_BITS));
	udelay(1);
	DSI_WRITE(DSI_PHY_TST_CTRL0, DPHY_CTRL0_PHY_TESTCLK_BITS);
	udelay(1);
	/* Since we are in DSI (not CSI2) mode here, start the PLL */
	actual_vco_freq = dphy_configure_pll(ref_freq, vco_freq);

	dphy_transaction(DPHY_CLK_PN_SWAP, LanePolarities);
	dphy_transaction(DPHY_D0_PN_SWAP, LanePolarities);
	dphy_transaction(DPHY_D1_PN_SWAP, LanePolarities);
	dphy_transaction(DPHY_D2_PN_SWAP, LanePolarities);
	dphy_transaction(DPHY_D3_PN_SWAP, LanePolarities);

	udelay(1);
	/* Unreset */
	DSI_WRITE(DSI_PHYRSTZ, DSI_PHYRSTZ_SHUTDOWNZ_BITS);
	udelay(1);
	DSI_WRITE(DSI_PHYRSTZ, (DSI_PHYRSTZ_SHUTDOWNZ_BITS | DSI_PHYRSTZ_RSTZ_BITS));
	udelay(1); /* so we can see PLL coming up? */

	return actual_vco_freq;
}

void CRP1DSIHostController::rp1dsi_mipicfg_setup (void)
{
	/* Select DSI rather than CSI-2 */
	CFG_WRITE(RPI_MIPICFG_CFG, 0);
	/* Enable DSIDMA interrupt only */
	CFG_WRITE(RPI_MIPICFG_INTE, RPI_MIPICFG_INTE_DSI_DMA_BITS);
}

unsigned long CRP1DSIHostController::rp1dsi_refclk_freq (void)
{
	return CMachineInfo::Get ()->GetGPIOClockSourceRate (GPIOClockSourceXOscillator);
}

int CRP1DSIHostController::rp1dsi_dpiclk_start (u32 byte_clock, unsigned int bpp, unsigned int lanes)
{
	/* Dummy clk_set_rate() to declare the actual DSI byte-clock rate */
	m_DPIClock.SetMIPIDSIByteClockRate (byte_clock);

	unsigned dpi_clock = (4 * lanes * byte_clock) / (bpp >> 1);
	if (!m_DPIClock.StartRate (dpi_clock))
	{
		return -1;
	}

	__LOGDBG ("Nominal Byte clock %u DPI clock %u", byte_clock, dpi_clock);

	return 0;
}

void CRP1DSIHostController::rp1dsi_dpiclk_stop (void)
{
	m_DPIClock.Stop ();
}

/* Choose the internal on-the-bus DPI format, and DSI packing flag. */
u32 CRP1DSIHostController::get_colorcode (enum mipi_dsi_pixel_format fmt)
{
	switch (fmt) {
	case MIPI_DSI_FMT_RGB666:
		return 0x104;
	case MIPI_DSI_FMT_RGB666_PACKED:
		return 0x003;
	case MIPI_DSI_FMT_RGB565:
		return 0x000;
	case MIPI_DSI_FMT_RGB888:
		return 0x005;
	}

	/* This should be impossible as the format is validated in
	 * rp1dsi_host_attach
	 */
	LOGWARN ("Invalid colour format configured for DSI");
	return 0x005;
}

/* Frequency limits for DPI, HS and LP clocks, and some magic numbers */
#define RP1DSI_DPI_MAX_KHZ     200000
#define RP1DSI_BYTE_CLK_MIN  10000000
#define RP1DSI_BYTE_CLK_MAX 187500000
#define RP1DSI_ESC_CLK_MAX   20000000
#define RP1DSI_TO_CLK_DIV        0x50
#define RP1DSI_LPRX_TO_VAL       0x40
#define RP1DSI_BTA_TO_VAL       0xd00

int CRP1DSIHostController::rp1dsi_dsi_setup (drm_display_mode const *mode)
{
	int cmdtim, ret;
	u32 timeout, mask, clkdiv;
	unsigned int bpp = mipi_dsi_pixel_format_to_bpp(m_display_format);
	u32 byte_clock = clamp((bpp * 125 * min(mode->clock, RP1DSI_DPI_MAX_KHZ)) / m_nDataLanes,
			       RP1DSI_BYTE_CLK_MIN, RP1DSI_BYTE_CLK_MAX);

	DSI_WRITE(DSI_PHY_IF_CFG, m_nDataLanes - 1);
	DSI_WRITE(DSI_DPI_CFG_POL, 0);
	DSI_WRITE(DSI_GEN_VCID, VC);
	DSI_WRITE(DSI_DPI_COLOR_CODING, get_colorcode(m_display_format));

	/*
	 * Flags to configure use of LP, EoTp, Burst Mode, Sync Events/Pulses.
	 * Note that Burst Mode implies Sync Events; the two flags need not be
	 * set concurrently, and in this RP1 variant *should not* both be set:
	 * doing so would (counter-intuitively) enable Sync Pulses and may fail
	 * if there is not sufficient time to return to LP11 state during HBP.
	 */
	mask =  DSI_VID_MODE_LP_HFP_EN  | DSI_VID_MODE_LP_HBP_EN |
		DSI_VID_MODE_LP_VACT_EN | DSI_VID_MODE_LP_VFP_EN |
		DSI_VID_MODE_LP_VBP_EN  | DSI_VID_MODE_LP_VSA_EN;
	if (m_display_flags & MIPI_DSI_MODE_LPM)
		mask |= DSI_VID_MODE_LP_CMD_EN;
	if (m_display_flags & MIPI_DSI_MODE_VIDEO_BURST)
		mask |= DSI_VID_MODE_BURST;
	else if (!(m_display_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE))
		mask |= DSI_VID_MODE_SYNC_EVENTS;
	else if (8 * m_nDataLanes > bpp)
		mask &= ~DSI_VID_MODE_LP_HBP_EN; /* PULSE && inexact DPICLK => fix HBP time */
	DSI_WRITE(DSI_VID_MODE_CFG, mask);
	DSI_WRITE(DSI_CMD_MODE_CFG,
		  (m_display_flags & MIPI_DSI_MODE_LPM) ? DSI_CMD_MODE_ALL_LP : 0);
	DSI_WRITE(DSI_PCKHDL_CFG,
		  DSI_PCKHDL_BTA_EN |
		  ((m_display_flags & MIPI_DSI_MODE_NO_EOT_PACKET) ? 0 : DSI_PCKHDL_EOTP_TX_EN));

	/* Select Command Mode */
	DSI_WRITE(DSI_MODE_CFG, 1);

	/* Set timeouts and clock dividers */
	timeout = (bpp * mode->htotal * mode->vdisplay) / (7 * RP1DSI_TO_CLK_DIV * m_nDataLanes);
	if (timeout > 0xFFFFu)
		timeout = 0;
	DSI_WRITE(DSI_TO_CNT_CFG, (timeout << 16) | RP1DSI_LPRX_TO_VAL);
	DSI_WRITE(DSI_BTA_TO_CNT, RP1DSI_BTA_TO_VAL);
	clkdiv = max(2u, 1u + byte_clock / RP1DSI_ESC_CLK_MAX); /* byte clocks per escape clock */
	DSI_WRITE(DSI_CLKMGR_CFG,
		  (RP1DSI_TO_CLK_DIV << 8) | clkdiv);

	/* Configure video timings */
	DSI_WRITE(DSI_VID_PKT_SIZE, mode->hdisplay);
	DSI_WRITE(DSI_VID_NUM_CHUNKS, 0);
	DSI_WRITE(DSI_VID_NULL_SIZE, 0);
	DSI_WRITE(DSI_VID_HSA_TIME,
		  (bpp * (mode->hsync_end - mode->hsync_start)) / (8 * m_nDataLanes));
	DSI_WRITE(DSI_VID_HBP_TIME,
		  (bpp * (mode->htotal - mode->hsync_end)) / (8 * m_nDataLanes));
	DSI_WRITE(DSI_VID_HLINE_TIME, (bpp * mode->htotal) / (8 * m_nDataLanes));
	DSI_WRITE(DSI_VID_VSA_LINES, (mode->vsync_end - mode->vsync_start));
	DSI_WRITE(DSI_VID_VBP_LINES, (mode->vtotal - mode->vsync_end));
	DSI_WRITE(DSI_VID_VFP_LINES, (mode->vsync_start - mode->vdisplay));
	DSI_WRITE(DSI_VID_VACTIVE_LINES, mode->vdisplay);

	/* Init PHY */
	byte_clock = dphy_init(rp1dsi_refclk_freq(), 8 * byte_clock) >> 3;

	DSI_WRITE(DSI_PHY_TMR_LPCLK_CFG,
		  (s_hsfreq_table[m_hsfreq_index].clk_lp2hs << DSI_PHY_TMR_LP2HS_LSB) |
		  (s_hsfreq_table[m_hsfreq_index].clk_hs2lp << DSI_PHY_TMR_HS2LP_LSB));
	DSI_WRITE(DSI_PHY_TMR_CFG,
		  (s_hsfreq_table[m_hsfreq_index].data_lp2hs << DSI_PHY_TMR_LP2HS_LSB) |
		  (s_hsfreq_table[m_hsfreq_index].data_hs2lp << DSI_PHY_TMR_HS2LP_LSB));

	/* Estimate how many LP bytes can be sent during vertical blanking (Databook 3.6.2.1) */
	cmdtim = mode->htotal;
	if (m_display_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE)
		cmdtim -= mode->hsync_end - mode->hsync_start;
	cmdtim = (bpp * cmdtim - 64) / (8 * m_nDataLanes);      /* byte clocks after HSS and EoTp */
	cmdtim -= s_hsfreq_table[m_hsfreq_index].data_hs2lp;
	cmdtim -= s_hsfreq_table[m_hsfreq_index].data_lp2hs;
	cmdtim = (cmdtim / clkdiv) - 24;                      /* escape clocks for commands */
	cmdtim = max(0, cmdtim >> 4);                         /* bytes (at 2 clocks per bit) */
	__LOGDBG ("Command time (outvact): %d", cmdtim);
	DSI_WRITE(DSI_DPI_LP_CMD_TIM, cmdtim << 16);

	/* Wait for PLL lock */
	for (timeout = (1 << 14); timeout != 0; --timeout) {
		usleep_range(10, 50);
		if (DSI_READ(DSI_PHY_STATUS) & (1 << 0))
			break;
	}
	if (timeout == 0) {
		LOGERR ("Timeout waiting for PLL");
		return -1;
	}

	DSI_WRITE(DSI_LPCLK_CTRL,
		  (m_display_flags & MIPI_DSI_CLOCK_NON_CONTINUOUS) ? 0x3 : 0x1);
	DSI_WRITE(DSI_PHY_TST_CTRL0, 0x2);
	DSI_WRITE(DSI_PWR_UP, 0x1);		/* power up */

	/* Now it should be safe to start the external DPI clock divider */
	ret = rp1dsi_dpiclk_start(byte_clock, bpp, m_nDataLanes);
	if (ret < 0)
	{
		LOGERR ("Cannot start DPI clock (%d)", ret);
		return ret;
	}

	/* Wait for all lane(s) to be in Stopstate */
	mask = (1 << 4);
	if (m_nDataLanes >= 2)
		mask |= (1 << 7);
	if (m_nDataLanes >= 3)
		mask |= (1 << 9);
	if (m_nDataLanes >= 4)
		mask |= (1 << 11);
	for (timeout = (1 << 10); timeout != 0; --timeout) {
		usleep_range(10, 50);
		if ((DSI_READ(DSI_PHY_STATUS) & mask) == mask)
			break;
	}
	if (timeout == 0) {
		LOGERR ("Time out waiting for lanes (%x %x)",
			mask, DSI_READ(DSI_PHY_STATUS));
		return -2;
	}

	return 0;
}

void CRP1DSIHostController::rp1dsi_dsi_send (u32 hdr, int len, const u8 *buf,
					   bool use_lpm, bool req_ack)
{
	u32 val;

	/* Wait for both FIFOs empty */
	for (val = 256; val > 0; --val) {
		if ((DSI_READ(DSI_CMD_PKT_STATUS) & 0xF) == 0x5)
			break;
		usleep_range(100, 150);
	}

	/*
	 * Update global configuration flags for LP/HS and ACK options.
	 * XXX It's not clear if having empty FIFOs (checked above and below) guarantees that
	 * the last command has completed and been ACKed, or how closely these control registers
	 * align with command/payload FIFO writes (as each is an independent clock-crossing)?
	 */
	val = DSI_READ(DSI_VID_MODE_CFG);
	if (use_lpm)
		val |= DSI_VID_MODE_LP_CMD_EN;
	else
		val &= ~DSI_VID_MODE_LP_CMD_EN;
	DSI_WRITE(DSI_VID_MODE_CFG, val);
	val = (use_lpm) ? DSI_CMD_MODE_ALL_LP : 0;
	if (req_ack)
		val |= DSI_CMD_MODE_ACK_RQST_EN;	// TODO? Read requires this
	DSI_WRITE(DSI_CMD_MODE_CFG, val);
	(void)DSI_READ(DSI_CMD_MODE_CFG);

	/* Write payload (in 32-bit words) and header */
	for (; len > 0; len -= 4) {
		val = *buf++;
		if (len > 1)
			val |= (*buf++) << 8;
		if (len > 2)
			val |= (*buf++) << 16;
		if (len > 3)
			val |= (*buf++) << 24;
		DSI_WRITE(DSI_GEN_PLD_DATA, val);
	}
	DSI_WRITE(DSI_GEN_HDR, hdr);

	/* Wait for both FIFOs empty */
	for (val = 256; val > 0; --val) {
		if ((DSI_READ(DSI_CMD_PKT_STATUS) & 0xF) == 0x5)
			break;
		usleep_range(100, 150);
	}
}

int CRP1DSIHostController::rp1dsi_dsi_recv (int len, u8 *buf)
{
	int i, j;
	u32 val;

	/* Wait until not busy and FIFO not empty */
	for (i = 1024; i > 0; --i) {
		val = DSI_READ(DSI_CMD_PKT_STATUS);
		if ((val & ((1 << 6) | (1 << 4))) == 0)
			break;
		usleep_range(100, 150);
	}
	if (!i) {
		LOGWARN ("Receive timed out");
		return -EIO;
	}

	for (i = 0; i < len; i += 4) {
		/* Read fifo must not be empty before all bytes are read */
		if (DSI_READ(DSI_CMD_PKT_STATUS) & (1 << 4))
			break;

		val = DSI_READ(DSI_GEN_PLD_DATA);
		for (j = 0; j < 4 && j + i < len; j++)
			*buf++ = val >> (8 * j);
	}

	return (i >= len) ? len : (i > 0) ? i : -EIO;
}

void CRP1DSIHostController::rp1dsi_dsi_stop (void)
{
	DSI_WRITE(DSI_MODE_CFG, 1);	/* Return to Command Mode */
	DSI_WRITE(DSI_LPCLK_CTRL, 2);	/* Stop the HS clock */
	DSI_WRITE(DSI_PWR_UP, 0x0);     /* Power down host controller */
	DSI_WRITE(DSI_PHYRSTZ, 0);      /* PHY into reset. */
	rp1dsi_dpiclk_stop();
}

void CRP1DSIHostController::rp1dsi_dsi_set_cmdmode (int mode)
{
	DSI_WRITE(DSI_MODE_CFG, mode);
}

unsigned CRP1DSIHostController::rp1dsi_dma_read(unsigned reg)
{
	return read32 (m_ulDMABase + reg);
}

void CRP1DSIHostController::rp1dsi_dma_write(unsigned reg, unsigned val)
{
	write32(m_ulDMABase + reg, val);
}

int CRP1DSIHostController::rp1dsi_dma_busy(void)
{
	return (rp1dsi_dma_read(DPI_DMA_STATUS) & 0xF8F) ? 1 : 0;
}

/* Choose the internal on-the-bus DPI format as expected by DSI Host. */
static u32 get_omask_oshift(enum mipi_dsi_pixel_format fmt, u32 *oshift)
{
	switch (fmt) {
	case MIPI_DSI_FMT_RGB565:
		*oshift = BITS(DPI_DMA_SHIFT_OR, 15) |
			  BITS(DPI_DMA_SHIFT_OG, 10) |
			  BITS(DPI_DMA_SHIFT_OB, 4);
		return BITS(DPI_DMA_OMASK_R, 0x3e0) |
		       BITS(DPI_DMA_OMASK_G, 0x3f0) |
		       BITS(DPI_DMA_OMASK_B, 0x3e0);
	case MIPI_DSI_FMT_RGB666_PACKED:
		*oshift = BITS(DPI_DMA_SHIFT_OR, 17) |
			  BITS(DPI_DMA_SHIFT_OG, 11) |
			  BITS(DPI_DMA_SHIFT_OB, 5);
		return BITS(DPI_DMA_OMASK_R, 0x3f0) |
		       BITS(DPI_DMA_OMASK_G, 0x3f0) |
		       BITS(DPI_DMA_OMASK_B, 0x3f0);
	case MIPI_DSI_FMT_RGB666:
		*oshift = BITS(DPI_DMA_SHIFT_OR, 21) |
			  BITS(DPI_DMA_SHIFT_OG, 13) |
			  BITS(DPI_DMA_SHIFT_OB, 5);
		return BITS(DPI_DMA_OMASK_R, 0x3f0) |
		       BITS(DPI_DMA_OMASK_G, 0x3f0) |
		       BITS(DPI_DMA_OMASK_B, 0x3f0);
	default:
		*oshift = BITS(DPI_DMA_SHIFT_OR, 23) |
			  BITS(DPI_DMA_SHIFT_OG, 15) |
			  BITS(DPI_DMA_SHIFT_OB, 7);
		return BITS(DPI_DMA_OMASK_R, 0x3fc) |
		       BITS(DPI_DMA_OMASK_G, 0x3fc) |
		       BITS(DPI_DMA_OMASK_B, 0x3fc);
	}
}

void CRP1DSIHostController::rp1dsi_dma_setup(u32 in_format, mipi_dsi_pixel_format out_format,
					     drm_display_mode const *mode)
{
	u32 oshift;
	unsigned i;

	/*
	 * Configure all DSI/DPI/DMA block registers, except base address.
	 * DMA will not actually start until a FB base address is specified
	 * using rp1dsi_dma_update().
	 */

	rp1dsi_dma_write(DPI_DMA_VISIBLE_AREA,
			 BITS(DPI_DMA_VISIBLE_AREA_ROWSM1, mode->vdisplay - 1) |
			 BITS(DPI_DMA_VISIBLE_AREA_COLSM1, mode->hdisplay - 1));

	rp1dsi_dma_write(DPI_DMA_SYNC_WIDTH,
			 BITS(DPI_DMA_SYNC_WIDTH_ROWSM1, mode->vsync_end - mode->vsync_start - 1) |
			 BITS(DPI_DMA_SYNC_WIDTH_COLSM1, mode->hsync_end - mode->hsync_start - 1));

	/* In the DPIDMA registers, "back porch" time includes sync width */
	rp1dsi_dma_write(DPI_DMA_BACK_PORCH,
			 BITS(DPI_DMA_BACK_PORCH_ROWSM1, mode->vtotal - mode->vsync_start - 1) |
			 BITS(DPI_DMA_BACK_PORCH_COLSM1, mode->htotal - mode->hsync_start - 1));

	rp1dsi_dma_write(DPI_DMA_FRONT_PORCH,
			 BITS(DPI_DMA_FRONT_PORCH_ROWSM1, mode->vsync_start - mode->vdisplay - 1) |
			 BITS(DPI_DMA_FRONT_PORCH_COLSM1, mode->hsync_start - mode->hdisplay - 1));

	/* Input to output pixel format conversion */
	for (i = 0; i < ARRAY_SIZE(my_formats); ++i) {
		if (my_formats[i].format == in_format)
			break;
	}
	if (i >= ARRAY_SIZE(my_formats)) {
		LOGERR ("Bad input format");
		i = 0;
	}
	rp1dsi_dma_write(DPI_DMA_IMASK, my_formats[i].mask);
	rp1dsi_dma_write(DPI_DMA_OMASK, get_omask_oshift(out_format, &oshift));
	rp1dsi_dma_write(DPI_DMA_SHIFT, my_formats[i].shift | oshift);
	if (out_format == MIPI_DSI_FMT_RGB888)
		rp1dsi_dma_write(DPI_DMA_RGBSZ, my_formats[i].rgbsz);
	else
		rp1dsi_dma_write(DPI_DMA_RGBSZ, my_formats[i].rgbsz & DPI_DMA_RGBSZ_BPP_MASK);

	rp1dsi_dma_write(DPI_DMA_QOS,
			 BITS(DPI_DMA_QOS_DQOS, 0x0) |
			 BITS(DPI_DMA_QOS_ULEV, 0xb) |
			 BITS(DPI_DMA_QOS_UQOS, 0x2) |
			 BITS(DPI_DMA_QOS_LLEV, 0x8) |
			 BITS(DPI_DMA_QOS_LQOS, 0x7));

	rp1dsi_dma_write(DPI_DMA_IRQ_FLAGS, -1);
	rp1dsi_dma_vblank_ctrl(1);

	i = rp1dsi_dma_busy();
	if (i)
		LOGERR ("Unexpectedly busy at start!");

	rp1dsi_dma_write(DPI_DMA_CONTROL,
			 BITS(DPI_DMA_CONTROL_ARM, (i == 0)) |
			 BITS(DPI_DMA_CONTROL_AUTO_REPEAT, 1) |
			 BITS(DPI_DMA_CONTROL_HIGH_WATER, 448) |
			 BITS(DPI_DMA_CONTROL_DEN_POL, 0) |
			 BITS(DPI_DMA_CONTROL_HSYNC_POL, 0) |
			 BITS(DPI_DMA_CONTROL_VSYNC_POL, 0) |
			 BITS(DPI_DMA_CONTROL_COLORM, 0) |
			 BITS(DPI_DMA_CONTROL_SHUTDN, 0) |
			 BITS(DPI_DMA_CONTROL_HBP_EN, 1) |
			 BITS(DPI_DMA_CONTROL_HFP_EN, 1) |
			 BITS(DPI_DMA_CONTROL_VBP_EN, 1) |
			 BITS(DPI_DMA_CONTROL_VFP_EN, 1) |
			 BITS(DPI_DMA_CONTROL_HSYNC_EN, 1) |
			 BITS(DPI_DMA_CONTROL_VSYNC_EN, 1));
}

void CRP1DSIHostController::rp1dsi_dma_update(uintptr addr, u32 offset, u32 stride)
{
	/*
	 * Update STRIDE, DMAH and DMAL only. When called after rp1dsi_dma_setup(),
	 * DMA starts immediately; if already running, the buffer will flip at
	 * the next vertical sync event.
	 */
	u64 a = addr + offset;

	rp1dsi_dma_write(DPI_DMA_DMA_STRIDE, stride);
	rp1dsi_dma_write(DPI_DMA_DMA_ADDR_H, a >> 32);
	rp1dsi_dma_write(DPI_DMA_DMA_ADDR_L, a & 0xFFFFFFFFu);
}

void CRP1DSIHostController::rp1dsi_dma_stop(void)
{
	assert (m_bDMARunning);

	/*
	 * Stop DMA by turning off the Auto-Repeat flag, and wait up to 100ms for
	 * the current and any queued frame to end. "Force drain" flags are not used,
	 * as they seem to prevent DMA from re-starting properly; it's safer to wait.
	 */
	u32 ctrl;

	ctrl = rp1dsi_dma_read(DPI_DMA_CONTROL);
	ctrl &= ~(DPI_DMA_CONTROL_ARM_MASK | DPI_DMA_CONTROL_AUTO_REPEAT_MASK);
	rp1dsi_dma_write(DPI_DMA_CONTROL, ctrl);

	msleep(100);
	if (m_bDMARunning)
		LOGERR ("Timed out waiting for idle");

	rp1dsi_dma_write(DPI_DMA_IRQ_EN, 0);
}

void CRP1DSIHostController::rp1dsi_dma_vblank_ctrl(int enable)
{
	rp1dsi_dma_write(DPI_DMA_IRQ_EN,
			 BITS(DPI_DMA_IRQ_EN_AFIFO_EMPTY, 1)      |
			 BITS(DPI_DMA_IRQ_EN_UNDERFLOW, 1)        |
			 BITS(DPI_DMA_IRQ_EN_DMA_READY, !!enable) |
			 BITS(DPI_DMA_IRQ_EN_MATCH_LINE, 4095));
}

void CRP1DSIHostController::rp1dsi_dma_isr (void *pParam)
{
	CRP1DSIHostController *pThis = static_cast<CRP1DSIHostController *> (pParam);
	assert (pThis);

	u32 u = pThis->rp1dsi_dma_read(DPI_DMA_IRQ_FLAGS);

	if (u) {
		// __LOGDBG ("IRQ (0x%X)", u);

		pThis->rp1dsi_dma_write(DPI_DMA_IRQ_FLAGS, u);

		if (u & DPI_DMA_IRQ_FLAGS_UNDERFLOW_MASK)
			LOGERR ("Underflow! (panics=0x%08x)",
				pThis->rp1dsi_dma_read(DPI_DMA_PANICS));
		if (u & DPI_DMA_IRQ_FLAGS_AFIFO_EMPTY_MASK) {
			pThis->m_bDMARunning = FALSE;
		} else if (   (u & DPI_DMA_IRQ_FLAGS_DMA_READY_MASK)
			   && pThis->m_pVBlankHandler) {
				(*pThis->m_pVBlankHandler) (pThis->m_pVBlankParam);
		}
	}
}
