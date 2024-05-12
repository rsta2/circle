//
// spimasterdma-rp1.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2024  R. Stange <rsta2@o2online.de>
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
#include <circle/memio.h>
#include <circle/sysconfig.h>
#include <circle/sched/scheduler.h>
#include <circle/logger.h>
#include <circle/macros.h>
#include <assert.h>

#define CLOCK_RATE_HZ	200000000U		// clk_sys

// RX channel must have higher priority
#define DMA_CHANNEL_RX	2			// TODO: allocate dynamically
#define DMA_CHANNEL_TX	3

// device registers (PSSI)
#define SPI_CTRLR0		(m_ulBaseAddress + 0x00)
	#define SPI_CTRLR0_FRF__SHIFT	4
		#define SPI_CTRLR0_FRF_SPI	0
	#define SPI_CTRLR0_SCPHA	BIT (6)
	#define SPI_CTRLR0_SCPOL	BIT (7)
	#define SPI_CTRLR0_TMOD__SHIFT	8
		#define SPI_CTRLR0_TMOD_TR	0
	#define SPI_CTRLR0_DFS32__SHIFT	16
		#define SPI_CTRLR0_DFS32_8BITS	7
#define SPI_SSIENR		(m_ulBaseAddress + 0x08)
	#define SPI_SSIENR_ENABLE	1
#define SPI_SER			(m_ulBaseAddress + 0x10)
	#define SPI_SER_CS0		BIT (0)
	#define SPI_SER_CS1		BIT (1)
	#define SPI_SER_CS2		BIT (2)
	#define SPI_SER_CS3		BIT (3)
#define SPI_BAUDR		(m_ulBaseAddress + 0x14)
	#define SPI_BAUDR_DIVIDER_MAX	0xFFFE
#define SPI_TXFLR		(m_ulBaseAddress + 0x20)
	#define SPI_FIFO_LENGTH		64
#define SPI_RXFLR		(m_ulBaseAddress + 0x24)
#define SPI_SR			(m_ulBaseAddress + 0x28)
	#define SPI_SR_TF_NFULL		BIT (1)
	#define SPI_SR_RF_NEMPTY	BIT (3)
#define SPI_IMR			(m_ulBaseAddress + 0x2C)
	#define SPI_IMR__MASK		0xFF
#define SPI_RISR		(m_ulBaseAddress + 0x34)
	#define SPI_INT_TXOI		BIT (1)
	#define SPI_INT_RXUI		BIT (2)
	#define SPI_INT_RXOI		BIT (3)
	#define SPI_INT_ERROR		(SPI_INT_TXOI | SPI_INT_RXUI | SPI_INT_RXOI)
#define SPI_ICR			(m_ulBaseAddress + 0x48)
#define SPI_DMACR		(m_ulBaseAddress + 0x4C)
	#define SPI_DMACR_RDMAE		BIT (0)
	#define SPI_DMACR_TDMAE		BIT (1)
#define SPI_DMATDLR		(m_ulBaseAddress + 0x50)
	#define SPI_TX_BURST_LEVEL	8
#define SPI_DMARDLR		(m_ulBaseAddress + 0x54)
	#define SPI_RX_BURST_LEVEL	1
#define SPI_VERSION		(m_ulBaseAddress + 0x5C)
	#define SPI_VERSION_SUPPORTED	0x3430322A
#define SPI_DR			(m_ulBaseAddress + 0x60)
#define SPI_RX_SAMPLE_DELAY	(m_ulBaseAddress + 0xF0)

#define DEVICES		6

static uintptr s_BaseAddress[DEVICES] =
{
	0x1F00050000UL,
	0x1F00054000UL,
	0x1F00058000UL,
	0x1F0005C000UL,
	0,
	0x1F00064000UL
};

#define GPIOS		5
	#define GPIO_MISO	0
	#define GPIO_MOSI	1
	#define GPIO_SCLK	2
	#define GPIO_CE0	3
	#define GPIO_CE1	4

#define NONE		10000

static unsigned s_GPIOConfig[DEVICES][GPIOS] =
{
	//MISO	MOSI	SCLK	CE0	CE1
	{ 9,	10,	11,	8,	7},
	{ 19,	20,	21,	18,	17},
	{ 1,	2,	3,	0,	24},
	{ 5,	6,	7,	4,	25},
	{ NONE,	NONE,	NONE,	NONE,	NONE},	// unused
	{ 13,	14,	15,	12,	26}
};

#define ALT_FUNC(device, gpio)		(  (device) <= 1		\
					 ? GPIOModeAlternateFunction0	\
					 : GPIOModeAlternateFunction8)

#define DREQS		2
	#define DREQ_RX		0
	#define DREQ_TX		1

static CDMAChannelRP1::TDREQ s_DREQ[DEVICES][DREQS] =
{
	{CDMAChannelRP1::DREQSourceSPI0RX, CDMAChannelRP1::DREQSourceSPI0TX},
	{CDMAChannelRP1::DREQSourceSPI1RX, CDMAChannelRP1::DREQSourceSPI1TX},
	{CDMAChannelRP1::DREQSourceSPI2RX, CDMAChannelRP1::DREQSourceSPI2TX},
	{CDMAChannelRP1::DREQSourceSPI3RX, CDMAChannelRP1::DREQSourceSPI3TX},
	{CDMAChannelRP1::DREQSourceNone, CDMAChannelRP1::DREQSourceNone},
	{CDMAChannelRP1::DREQSourceSPI5RX, CDMAChannelRP1::DREQSourceSPI5TX}
};

LOGMODULE ("spi-rp1");

CSPIMasterDMA::CSPIMasterDMA (CInterruptSystem *pInterruptSystem,
			      unsigned nClockSpeed, unsigned CPOL, unsigned CPHA,
			      boolean bDMAChannelLite, unsigned nDevice)
:	m_nDevice (nDevice),
	m_ulBaseAddress (0),
	m_bValid (FALSE),
	m_TxDMA (DMA_CHANNEL_TX, pInterruptSystem),
	m_RxDMA (DMA_CHANNEL_RX, pInterruptSystem),
	m_pCompletionRoutine (nullptr)
{
	if (   nDevice >= DEVICES
	    || s_GPIOConfig[nDevice][0] >= GPIO_PINS)
	{
		return;
	}

	m_ulBaseAddress = s_BaseAddress[nDevice];
	assert (m_ulBaseAddress);
	assert (read32 (SPI_VERSION) == SPI_VERSION_SUPPORTED);

	m_MISO.AssignPin (s_GPIOConfig[nDevice][GPIO_MISO]);
	m_MISO.SetMode (ALT_FUNC (nDevice, GPIO_MISO));
	m_MISO.SetPullMode (GPIOPullModeOff);

	m_MOSI.AssignPin (s_GPIOConfig[nDevice][GPIO_MOSI]);
	m_MOSI.SetMode (ALT_FUNC (nDevice, GPIO_MOSI));
	m_MOSI.SetPullMode (GPIOPullModeOff);
	m_MOSI.SetDriveStrength (GPIODriveStrength12mA);
	m_MOSI.SetSlewRate (GPIOSlewRateFast);

	m_SCLK.AssignPin (s_GPIOConfig[nDevice][GPIO_SCLK]);
	m_SCLK.SetMode (ALT_FUNC (nDevice, GPIO_SCLK));
	m_SCLK.SetPullMode (GPIOPullModeOff);
	m_SCLK.SetDriveStrength (GPIODriveStrength12mA);
	m_SCLK.SetSlewRate (GPIOSlewRateFast);

	m_CE0.AssignPin (s_GPIOConfig[nDevice][GPIO_CE0]);
	m_CE0.SetMode (ALT_FUNC (nDevice, GPIO_CE0));
	m_CE0.SetPullMode (GPIOPullModeUp);

	m_CE1.AssignPin (s_GPIOConfig[nDevice][GPIO_CE1]);
	m_CE1.SetMode (ALT_FUNC (nDevice, GPIO_CE1));
	m_CE1.SetPullMode (GPIOPullModeUp);

	m_bValid = TRUE;

	SetClock (nClockSpeed);
	SetMode (CPOL, CPHA);
}

CSPIMasterDMA::~CSPIMasterDMA (void)
{
	if (m_bValid)
	{
		m_TxDMA.Cancel ();
		m_RxDMA.Cancel ();

		assert (m_ulBaseAddress);
		write32 (SPI_SSIENR, 0);
		write32 (SPI_BAUDR, 0);

		m_MISO.SetMode (GPIOModeNone);
		m_MOSI.SetMode (GPIOModeNone);
		m_SCLK.SetMode (GPIOModeNone);
		m_CE0.SetMode (GPIOModeNone);
		m_CE1.SetMode (GPIOModeNone);

		m_bValid = FALSE;
	}

	m_ulBaseAddress = 0;
}

boolean CSPIMasterDMA::Initialize (void)
{
	if (!m_bValid)
	{
		return FALSE;
	}

	assert (m_ulBaseAddress);

	write32 (SPI_SSIENR, 0);

	write32 (SPI_IMR, read32 (SPI_IMR) & ~SPI_IMR__MASK);
	read32 (SPI_ICR);

	write32 (SPI_SER, 0);

	write32 (SPI_SSIENR, SPI_SSIENR_ENABLE);

	write32 (SPI_RX_SAMPLE_DELAY, 0);

	return TRUE;
}

void CSPIMasterDMA::SetClock (unsigned nClockSpeed)
{
	assert (m_bValid);
	assert (m_ulBaseAddress);
	assert (nClockSpeed > CLOCK_RATE_HZ / 0xFFFE + 1);
	assert (nClockSpeed <= CLOCK_RATE_HZ);

	write32 (SPI_BAUDR, ((CLOCK_RATE_HZ + nClockSpeed - 1) / nClockSpeed + 1) & 0xFFFE);
}

void CSPIMasterDMA::SetMode (unsigned CPOL, unsigned CPHA)
{
	assert (m_bValid);
	assert (m_ulBaseAddress);
	assert (CPOL <= 1);
	assert (CPHA <= 1);

	u32 nCtrlr0 =   SPI_CTRLR0_FRF_SPI << SPI_CTRLR0_FRF__SHIFT
		      | SPI_CTRLR0_TMOD_TR << SPI_CTRLR0_TMOD__SHIFT
		      | SPI_CTRLR0_DFS32_8BITS << SPI_CTRLR0_DFS32__SHIFT;

	if (CPOL)
	{
		nCtrlr0 |= SPI_CTRLR0_SCPOL;
	}

	if (CPHA)
	{
		nCtrlr0 |= SPI_CTRLR0_SCPHA;
	}

	write32 (SPI_CTRLR0, nCtrlr0);

}

void CSPIMasterDMA::SetCompletionRoutine (TSPICompletionRoutine *pRoutine, void *pParam)
{
	assert (m_pCompletionRoutine == 0);
	m_pCompletionRoutine = pRoutine;
	assert (m_pCompletionRoutine != 0);

	m_pCompletionParam = pParam;
}

void CSPIMasterDMA::StartWriteRead (unsigned nChipSelect, const void *pWriteBuffer,
				    void *pReadBuffer, unsigned nCount)
{
	assert (pWriteBuffer);
	m_TxDMA.SetupIOWrite (SPI_DR, pWriteBuffer, nCount, s_DREQ[m_nDevice][DREQ_TX], 1);

	assert (pReadBuffer);
	m_RxDMA.SetupIORead (pReadBuffer, SPI_DR, nCount, s_DREQ[m_nDevice][DREQ_RX], 1);

	write32 (SPI_DMARDLR, SPI_RX_BURST_LEVEL - 1);
	write32 (SPI_DMATDLR, SPI_TX_BURST_LEVEL);

	switch (nChipSelect)
	{
	case 0:		write32 (SPI_SER, SPI_SER_CS0);		break;
	case 1:		write32 (SPI_SER, SPI_SER_CS1);		break;
	case 2:		write32 (SPI_SER, SPI_SER_CS2);		break;
	default:
	case 3:		write32 (SPI_SER, SPI_SER_CS3);		break;
	}

	write32 (SPI_DMACR, SPI_DMACR_TDMAE | SPI_DMACR_RDMAE);

	// TODO: write32 (DW_SPI_IMR, read32 (SPI_IMR) | DW_SPI_INT_RXUI | DW_SPI_INT_RXOI);

	write32 (SPI_SSIENR, SPI_SSIENR_ENABLE);

	m_bTxStatus = FALSE;

	m_RxDMA.SetCompletionRoutine (DMACompletionStub, this);
	m_TxDMA.SetCompletionRoutine (DMACompletionStub, this);

	m_RxDMA.Start ();
	m_TxDMA.Start ();
}

void CSPIMasterDMA::DMACompletionRoutine (unsigned nChannel, boolean bStatus)
{
	if (nChannel == DMA_CHANNEL_TX)
	{
		assert (!m_bTxStatus);
		m_bTxStatus = bStatus;
	}
	else
	{
		assert (nChannel == DMA_CHANNEL_RX);

		write32 (SPI_DMACR, 0);

		write32 (SPI_SER, 0);

		TSPICompletionRoutine *pCompletionRoutine = m_pCompletionRoutine;
		m_pCompletionRoutine = nullptr;

		if (pCompletionRoutine)
		{
			(*pCompletionRoutine) (bStatus && m_bTxStatus, m_pCompletionParam);
		}
	}
}

void CSPIMasterDMA::DMACompletionStub (unsigned nChannel, unsigned nBuffer,
				       boolean bStatus, void *pParam)
{
	CSPIMasterDMA *pThis = static_cast<CSPIMasterDMA *> (pParam);
	assert (pThis != 0);

	pThis->DMACompletionRoutine (nChannel, bStatus);
}

int CSPIMasterDMA::WriteReadSync (unsigned nChipSelect, const void *pWriteBuffer, void *pReadBuffer,
				  unsigned nCount)
{
	assert (m_bValid);
	assert (m_ulBaseAddress);

	assert (pWriteBuffer || pReadBuffer);
	const u8 *pWritePtr = static_cast<const u8 *> (pWriteBuffer);
	u8 *pReadPtr = static_cast<u8 *> (pReadBuffer);

	assert (nCount);
	unsigned nWriteCount = nCount;
	unsigned nReadCount = nCount;

	int nResult = nCount;

	switch (nChipSelect)
	{
	case 0:		write32 (SPI_SER, SPI_SER_CS0);		break;
	case 1:		write32 (SPI_SER, SPI_SER_CS1);		break;
	case 2:		write32 (SPI_SER, SPI_SER_CS2);		break;
	default:
	case 3:		write32 (SPI_SER, SPI_SER_CS3);		break;
	}

	write32 (SPI_SSIENR, SPI_SSIENR_ENABLE);

	while (nReadCount || nWriteCount)
	{
		unsigned nBytes = SPI_FIFO_LENGTH - read32 (SPI_TXFLR);
		if (nWriteCount < nBytes)
		{
			nBytes = nWriteCount;
		}

		while (nBytes--)
		{
			write32 (SPI_DR, pWritePtr ? *pWritePtr++ : 0);

			nWriteCount--;
		}

#ifdef NO_BUSY_WAIT
		CScheduler::Get ()->Yield ();
#endif

		nBytes = read32 (SPI_RXFLR);
		if (nReadCount < nBytes)
		{
			nBytes = nReadCount;
		}

		while (nBytes--)
		{
			u8 uchData = static_cast<u8> (read32 (SPI_DR));
			if (pReadPtr)
			{
				*pReadPtr++ = uchData;
			}

			nReadCount--;
		}

		u32 nStatus = read32 (SPI_RISR);
		if (nStatus & SPI_INT_ERROR)
		{
			LOGWARN ("SPI error 0x%X", nStatus);

			read32 (SPI_ICR);

			nResult = -1;

			break;
		}
	}

	write32 (SPI_SER, 0);

	return nResult;
}
