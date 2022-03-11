//
/// \file smimaster.cpp
//
// Driver for the Second Memory Interface
// Original development by Sebastien Nicolas <seba1978@gmx.de>
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2022  R. Stange <rsta2@o2online.de>
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
#include <circle/smimaster.h>
#include <circle/bcm2835.h>
#include <circle/memio.h>
#include <circle/timer.h>
#include <assert.h>

// Registers - ref the linux driver (bcm2835_smi.h) for documentation
#define ARM_SMI_BASE	(ARM_IO_BASE + 0x600000)
#define ARM_SMI_CS		(ARM_SMI_BASE + 0x00)	// Control & status
#define ARM_SMI_L		(ARM_SMI_BASE + 0x04)	// Transfer length/count
#define ARM_SMI_A		(ARM_SMI_BASE + 0x08)	// Address
#define ARM_SMI_D		(ARM_SMI_BASE + 0x0c)	// Data
#define ARM_SMI_DSR0	(ARM_SMI_BASE + 0x10)	// Read settings device 0
#define ARM_SMI_DSW0	(ARM_SMI_BASE + 0x14)	// Write settings device 0
#define ARM_SMI_DSR1	(ARM_SMI_BASE + 0x18)	// Read settings device 1
#define ARM_SMI_DSW1	(ARM_SMI_BASE + 0x1c)	// Write settings device 1
#define ARM_SMI_DSR2	(ARM_SMI_BASE + 0x20)	// Read settings device 2
#define ARM_SMI_DSW2	(ARM_SMI_BASE + 0x24)	// Write settings device 2
#define ARM_SMI_DSR3	(ARM_SMI_BASE + 0x28)	// Read settings device 3
#define ARM_SMI_DSW3	(ARM_SMI_BASE + 0x2c)	// Write settings device 3
#define ARM_SMI_DMC		(ARM_SMI_BASE + 0x30)	// DMA control
#define ARM_SMI_DCS		(ARM_SMI_BASE + 0x34)	// Direct control/status
#define ARM_SMI_DCA		(ARM_SMI_BASE + 0x38)	// Direct address
#define ARM_SMI_DCD		(ARM_SMI_BASE + 0x3c)	// Direct data
#define ARM_SMI_FD		(ARM_SMI_BASE + 0x40)	// FIFO debug

// CS Register
#define CS_ENABLE		(1 << 0)
#define CS_DONE			(1 << 1)
//#define CS_ACTIVE		(1 << 2)	// TODO: name clash with <circle/dmacommon.h>
#define CS_START		(1 << 3)
#define CS_CLEAR		(1 << 4)
#define CS_WRITE		(1 << 5)
//#define CS_UNUSED1__SHIFT	6
#define CS_TEEN			(1 << 8)
#define CS_INTD			(1 << 9)
#define CS_INTT			(1 << 10)
#define CS_INTR			(1 << 11)
#define CS_PVMODE		(1 << 12)
#define CS_SETERR		(1 << 13)
#define CS_PXLDAT		(1 << 14)
#define CS_EDREQ		(1 << 15)
//#define CS_UNUSED2__SHIFT	16
#define CS_PRDY			(1 << 24)
#define CS_AFERR		(1 << 25)
#define CS_TXW			(1 << 26)
#define CS_RXR			(1 << 27)
#define CS_TXD			(1 << 28)
#define CS_RXD			(1 << 29)
#define CS_TXE			(1 << 30)
#define CS_RXF			(1 << 31)

// Address Register
#define A_ADDR__SHIFT		0
//#define A_UNUSED__SHIFT	6
#define A_DEV__SHIFT		8

// Read Settings Register
#define DSR_RSTROBE__SHIFT	0
#define DSR_RDREQ			(1 << 7)
#define DSR_RPACE__SHIFT	8
#define DSR_RPACEALL		(1 << 15)
#define DSR_RHOLD__SHIFT	16
#define DSR_FSETUP			(1 << 22)
#define DSR_MODE68			(1 << 23)
#define DSR_RSETUP__SHIFT	24
#define DSR_RWIDTH__SHIFT	30

// Write Settings Register
#define DSW_WSTROBE__SHIFT	0
#define DSW_WDREQ			(1 << 7)
#define DSW_WPACE__SHIFT	8
#define DSW_WPACEALL		(1 << 15)
#define DSW_WHOLD__SHIFT	16
#define DSW_WSWAP			(1 << 22)
#define DSW_WFORMAT			(1 << 23)
#define DSW_WSETUP__SHIFT	24
#define DSW_WWIDTH__SHIFT	30

// DMA Control Register
#define DMC_REQW__SHIFT		0
#define DMC_REQR__SHIFT		6
#define DMC_PANICW__SHIFT	12
#define DMC_PANICR__SHIFT	18
#define DMC_DMAP			(1 << 24)
//#define DMC_UNUSED__SHIFT	27
#define DMC_DMAEN			(1 << 28)

// Direct Control/Status Register
#define DCS_ENABLE			(1 << 0)
#define DCS_START			(1 << 1)
#define DCS_DONE			(1 << 2)
#define DCS_WRITE			(1 << 3)

// Direct Control Address Register
#define DCA_ADDR__SHIFT		0
//#define DCA_UNUSED__SHIFT	6
#define DCA_DEV__SHIFT		8

// FIFO Debug Register
#define FD_FCNT__SHIFT		0
//#define FD_UNUSED__SHIFT	6
#define FD_FLVL__SHIFT		8


// DMA
#define DMA_REQUEST_THRESH  2
#define DMA_PANIC_LEVEL		8

// Clock
#define CM_SMICTL_FLIP (1 << 8)
#define CM_SMICTL_BUSY (1 << 7)
#define CM_SMICTL_KILL (1 << 5)
#define CM_SMICTL_ENAB (1 << 4)
#define CM_SMICTL_SRC_MASK (0xf)
#define CM_SMICTL_SRC_OFFS (0)

#define CM_SMIDIV_DIVI_MASK (0xf <<  12)
#define CM_SMIDIV_DIVI_OFFS (12)
#define CM_SMIDIV_DIVF_MASK (0xff << 4)
#define CM_SMIDIV_DIVF_OFFS (4)




CSMIMaster::CSMIMaster(unsigned nSDLinesMask, boolean bUseAddressPins) :
	m_nSDLinesMask (nSDLinesMask),
	m_bUseAddressPins (bUseAddressPins),
	m_txDMA (DMA_CHANNEL_LITE /*DMA_CHANNEL_NORMAL*/),
	m_pDMABuffer (0)
{
	if (m_bUseAddressPins) {
		for (unsigned i = 0 ; i < SMI_NUM_ADDRESS_LINES ; i++) {
			m_addressGpios[i].AssignPin(GPIO_FOR_SAx(i));
			m_addressGpios[i].SetMode(GPIOModeAlternateFunction1);
		}
	}
	for (unsigned i = 0 ; i < SMI_NUM_DATA_LINES ; i++) {
		if (m_nSDLinesMask & (1 << i)) {
			m_dataGpios[i].AssignPin(GPIO_FOR_SDx(i));
			m_dataGpios[i].SetMode(GPIOModeAlternateFunction1);
		}
	}
}

CSMIMaster::~CSMIMaster (void)
{
	if (m_bUseAddressPins) {
		for (unsigned i = 0 ; i < SMI_NUM_ADDRESS_LINES ; i++) {
			m_addressGpios[i].SetMode(GPIOModeInput);
		}
	}
	for (unsigned i = 0 ; i < SMI_NUM_DATA_LINES ; i++) {
		if (m_nSDLinesMask & (1 << i)) {
			m_dataGpios[i].SetMode(GPIOModeInput);
		}
	}
	PeripheralEntry();
	write32(ARM_SMI_CS, 0);
	CTimer::Get ()->usDelay (50);
	PeripheralExit();
}

unsigned CSMIMaster::GetSDLinesMask () {
	return m_nSDLinesMask;
}


unsigned CSMIMaster::Read () {
	PeripheralEntry();
	write32(ARM_SMI_CS, read32(ARM_SMI_CS) | CS_CLEAR | CS_AFERR); // clear FIFO and FIFO error flag
	write32(ARM_SMI_DCS, DCS_ENABLE | DCS_START); // enable and start direct mode for reading
	while ((read32(ARM_SMI_DCS) & DCS_DONE) == 0) {} // wait until completion
	unsigned val = read32(ARM_SMI_DCD);
	PeripheralExit();
	return val;
}


void CSMIMaster::Write (unsigned nValue) {
	PeripheralEntry();
	write32(ARM_SMI_CS, read32(ARM_SMI_CS) | CS_CLEAR | CS_AFERR); // clear FIFO and FIFO error flag
	write32(ARM_SMI_DCS, DCS_ENABLE | DCS_WRITE); // enable direct mode for writing
	write32(ARM_SMI_DCD, nValue);
	write32(ARM_SMI_DCS, read32(ARM_SMI_DCS) | DCS_START); // start the cycle
	while ((read32(ARM_SMI_DCS) & DCS_DONE) == 0) {} // wait until completion
	PeripheralExit();
}


void CSMIMaster::WriteDMA(boolean bWaitForCompletion)
{
	assert (m_pDMABuffer != 0);
	m_txDMA.SetupIOWrite (ARM_SMI_D, m_pDMABuffer, m_nLength, DREQSourceSMI);
	m_txDMA.Start();
	PeripheralEntry();
	write32(ARM_SMI_CS, read32(ARM_SMI_CS) | CS_START);
	PeripheralExit();
	if (bWaitForCompletion) m_txDMA.Wait();
}

void CSMIMaster::SetupTiming(TSMIDataWidth nWidth, unsigned nCycle_ns, unsigned nSetup, unsigned nStrobe, unsigned nHold, unsigned nPace, unsigned nDevice)
{
	uintptr readReg, writeReg;
	switch (nDevice) {
	case 0:
		readReg = ARM_SMI_DSR0;
		writeReg = ARM_SMI_DSW0;
		break;
	case 1:
		readReg = ARM_SMI_DSR1;
		writeReg = ARM_SMI_DSW1;
		break;
	case 2:
		readReg = ARM_SMI_DSR2;
		writeReg = ARM_SMI_DSW2;
		break;
	case 3:
		readReg = ARM_SMI_DSR3;
		writeReg = ARM_SMI_DSW3;
		break;
	default:
		assert(0);
		readReg = ARM_SMI_DSR0;		// to keep the compiler quiet
		writeReg = ARM_SMI_DSW0;
		break;
	}
	PeripheralEntry ();

	// Reset SMI regs
	write32(ARM_SMI_CS, 0);
	write32(ARM_SMI_L, 0);
	write32(ARM_SMI_A, 0);
	write32(readReg, 0);
	write32(writeReg, 0);
	write32(ARM_SMI_DCS, 0);
	write32(ARM_SMI_DCA, 0);

	PeripheralExit ();

	PeripheralEntry ();		// switch to SMI clock device

	// Initialise SMI clock
	u32 divi = nCycle_ns / 2;
	if (read32(ARM_CM_SMICTL) != divi << CM_SMIDIV_DIVI_OFFS) {
		write32(ARM_CM_SMICTL, ARM_CM_PASSWD | CM_SMICTL_KILL);
		CTimer::Get ()->usDelay (10);
		while (read32(ARM_CM_SMICTL) & CM_SMICTL_BUSY) {}
		CTimer::Get ()->usDelay (10);
		write32(ARM_CM_SMIDIV, ARM_CM_PASSWD | (divi << CM_SMIDIV_DIVI_OFFS));
		CTimer::Get ()->usDelay (10);
		write32(ARM_CM_SMICTL, ARM_CM_PASSWD | 6 | CM_SMICTL_ENAB);
		CTimer::Get ()->usDelay (10);
		while ((read32(ARM_CM_SMICTL) & CM_SMICTL_BUSY) == 0) {}
		CTimer::Get ()->usDelay (100);
	}

	PeripheralExit ();

	PeripheralEntry ();		// switch back to SMI device

	// Initialise SMI with data width, time step, and setup/hold/strobe counts
	u32 tmp = read32(ARM_SMI_CS);
	if (tmp & CS_SETERR) write32(ARM_SMI_CS, tmp | CS_SETERR); // clear error flag
	u32 timing = (nWidth << DSR_RWIDTH__SHIFT) | (nSetup << DSR_RSETUP__SHIFT) | (nStrobe << DSR_RSTROBE__SHIFT) | (nHold << DSR_RHOLD__SHIFT) | (nPace << DSR_RPACE__SHIFT);
	write32(writeReg, timing);
	write32(readReg, timing); // DSR and DSW have the same fields, so we can reuse timing above

	PeripheralExit ();
}

void CSMIMaster::SetupDMA(void *pDMABuffer, unsigned nLength)
{
	assert (pDMABuffer != 0);
	m_pDMABuffer = pDMABuffer;
	m_nLength = nLength;

	PeripheralEntry ();
	write32(ARM_SMI_DMC, (DMA_REQUEST_THRESH << DMC_REQW__SHIFT) | (DMA_REQUEST_THRESH << DMC_REQR__SHIFT) | (DMA_PANIC_LEVEL << DMC_PANICW__SHIFT) | (DMA_PANIC_LEVEL << DMC_PANICR__SHIFT) | DMC_DMAEN);
	write32(ARM_SMI_CS, read32(ARM_SMI_CS) | CS_ENABLE | CS_CLEAR | CS_PXLDAT); // CS_PXLDAT packs the 8 or 16 bit data into 32-bit double-words
	write32(ARM_SMI_L, nLength);
	write32(ARM_SMI_CS, read32(ARM_SMI_CS) | CS_WRITE);
	PeripheralExit ();
}

void CSMIMaster::SetDeviceAndAddress (unsigned nDevice, unsigned nAddr) {
	assert(nDevice <= 3);
	assert(nAddr <= 0b111111);
	unsigned val = (nAddr << A_ADDR__SHIFT) | (nDevice << A_DEV__SHIFT);

	PeripheralEntry ();
	write32(ARM_SMI_A, val);
	write32(ARM_SMI_DCA, val);
	PeripheralExit ();
}
