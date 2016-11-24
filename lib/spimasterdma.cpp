//
// spimasterdma.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016  R. Stange <rsta2@o2online.de>
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
#include <circle/spimasterdma.h>
#include <circle/bcm2835.h>
#include <circle/memio.h>
#include <circle/machineinfo.h>
#include <circle/synchronize.h>
#include <assert.h>

// CS Register
#define CS_LEN_LONG	(1 << 25)
#define CS_DMA_LEN	(1 << 24)
#define CS_CSPOL2	(1 << 23)
#define CS_CSPOL1	(1 << 22)
#define CS_CSPOL0	(1 << 21)
#define CS_RXF		(1 << 20)
#define CS_RXR		(1 << 19)
#define CS_TXD		(1 << 18)
#define CS_RXD		(1 << 17)
#define CS_DONE		(1 << 16)
#define CS_LEN		(1 << 13)
#define CS_REN		(1 << 12)
#define CS_ADCS		(1 << 11)
#define CS_INTR		(1 << 10)
#define CS_INTD		(1 << 9)
#define CS_DMAEN	(1 << 8)
#define CS_TA		(1 << 7)
#define CS_CSPOL	(1 << 6)
#define CS_CLEAR_RX	(1 << 5)
#define CS_CLEAR_TX	(1 << 4)
#define CS_CPOL__SHIFT	3
#define CS_CPHA__SHIFT	2
#define CS_CS		(1 << 0)
#define CS_CS__SHIFT	0

CSPIMasterDMA::CSPIMasterDMA (CInterruptSystem *pInterruptSystem,
			      unsigned nClockSpeed, unsigned CPOL, unsigned CPHA)
:	m_nClockSpeed (nClockSpeed),
	m_CPOL (CPOL),
	m_CPHA (CPHA),
	m_TxDMA (DMA_CHANNEL_SPI_TX, pInterruptSystem),
	m_RxDMA (DMA_CHANNEL_SPI_RX, pInterruptSystem),
	m_SCLK (11, GPIOModeAlternateFunction0),
	m_MOSI (10, GPIOModeAlternateFunction0),
	m_MISO ( 9, GPIOModeAlternateFunction0),
	m_CE0  ( 8, GPIOModeAlternateFunction0),
	m_CE1  ( 7, GPIOModeAlternateFunction0),
	m_nCoreClockRate (CMachineInfo::Get ()->GetClockRate (CLOCK_ID_CORE)),
	m_pCompletionRoutine (0)
{
	assert (m_nCoreClockRate > 0);
}

CSPIMasterDMA::~CSPIMasterDMA (void)
{
}

boolean CSPIMasterDMA::Initialize (void)
{
	PeripheralEntry ();

	assert (4000 <= m_nClockSpeed && m_nClockSpeed <= 125000000);
	write32 (ARM_SPI0_CLK, m_nCoreClockRate / m_nClockSpeed);

	assert (m_CPOL <= 1);
	assert (m_CPHA <= 1);
	write32 (ARM_SPI0_CS, (m_CPOL << CS_CPOL__SHIFT) | (m_CPHA << CS_CPHA__SHIFT));

	PeripheralExit ();

	return TRUE;
}

void CSPIMasterDMA::SetClock (unsigned nClockSpeed)
{
	m_nClockSpeed = nClockSpeed;

	PeripheralEntry ();

	assert (4000 <= m_nClockSpeed && m_nClockSpeed <= 125000000);
	write32 (ARM_SPI0_CLK, m_nCoreClockRate / m_nClockSpeed);

	PeripheralExit ();
}

void CSPIMasterDMA::SetMode (unsigned CPOL, unsigned CPHA)
{
	m_CPOL = CPOL;
	m_CPHA = CPHA;

	PeripheralEntry ();

	assert (m_CPOL <= 1);
	assert (m_CPHA <= 1);
	write32 (ARM_SPI0_CS, (m_CPOL << CS_CPOL__SHIFT) | (m_CPHA << CS_CPHA__SHIFT));

	PeripheralExit ();
}

void CSPIMasterDMA::SetCompletionRoutine (TSPICompletionRoutine *pRoutine, void *pParam)
{
	assert (m_pCompletionRoutine == 0);
	m_pCompletionRoutine = pRoutine;
	assert (m_pCompletionRoutine != 0);

	m_pCompletionParam = pParam;
}

void CSPIMasterDMA::StartWriteRead (unsigned	nChipSelect,
				    const void *pWriteBuffer,
				    void       *pReadBuffer,
				    unsigned	nCount)
{
	assert (pWriteBuffer != 0);
	m_TxDMA.SetupIOWrite (ARM_SPI0_FIFO, pWriteBuffer, nCount, DREQSourceSPITX);

	assert (pReadBuffer != 0);
	m_RxDMA.SetupIORead (pReadBuffer, ARM_SPI0_FIFO, nCount, DREQSourceSPIRX);

	PeripheralEntry ();

	assert (nCount <= 0xFFFF);
	write32 (ARM_SPI0_DLEN, nCount);

	assert (nChipSelect <= 1);
	write32 (ARM_SPI0_CS,   (read32 (ARM_SPI0_CS) & ~CS_CS)
			      | (nChipSelect << CS_CS__SHIFT)
			      | CS_CLEAR_RX | CS_CLEAR_TX
			      | CS_DMAEN | CS_ADCS
			      | CS_TA);

	PeripheralExit ();

	m_RxDMA.SetCompletionRoutine (DMACompletionStub, this);

	m_RxDMA.Start ();
	m_TxDMA.Start ();
}

void CSPIMasterDMA::DMACompletionRoutine (boolean bStatus)
{
	PeripheralEntry ();
	write32 (ARM_SPI0_CS, read32 (ARM_SPI0_CS) & ~CS_TA);
	PeripheralExit ();

	TSPICompletionRoutine *pCompletionRoutine = m_pCompletionRoutine;
	m_pCompletionRoutine = 0;

	if (pCompletionRoutine != 0)
	{
		(*pCompletionRoutine) (bStatus && m_TxDMA.GetStatus (), m_pCompletionParam);
	}
}

void CSPIMasterDMA::DMACompletionStub (unsigned nChannel, boolean bStatus, void *pParam)
{
	assert (nChannel == DMA_CHANNEL_SPI_RX);

	CSPIMasterDMA *pThis = (CSPIMasterDMA *) pParam;
	assert (pThis != 0);

	pThis->DMACompletionRoutine (bStatus);
}
