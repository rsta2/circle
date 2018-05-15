//
// i2ssoundbasedevice.cpp
//
// Supports:
//	BCM283x I2S output
//	two 24-bit audio channels
//	sample rate up to 192 KHz
//	tested with PCM5102A DAC only
//
// References:
//	https://www.raspberrypi.org/forums/viewtopic.php?f=44&t=8496
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2018  R. Stange <rsta2@o2online.de>
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
#include <circle/i2ssoundbasedevice.h>
#include <circle/devicenameservice.h>
#include <circle/bcm2835.h>
#include <circle/bcm2835int.h>
#include <circle/memio.h>
#include <circle/timer.h>
#include <circle/synchronize.h>
#include <circle/util.h>
#include <assert.h>

#define CHANS			2			// 2 I2S stereo channels
#define CHANLEN			32			// width of a channel slot in bits

//
// PCM / I2S registers
//
#define CS_A_STBY		(1 << 25)
#define CS_A_SYNC		(1 << 24)
#define CS_A_TXE		(1 << 21)
#define CS_A_TXD		(1 << 19)
#define CS_A_TXW		(1 << 17)
#define CS_A_TXERR		(1 << 15)
#define CS_A_TXSYNC		(1 << 13)
#define CS_A_DMAEN		(1 << 9)
#define CS_A_TXTHR__SHIFT	5
#define CS_A_RXCLR		(1 << 4)
#define CS_A_TXCLR		(1 << 3)
#define CS_A_TXON		(1 << 2)
#define CS_A_EN			(1 << 0)

#define MODE_A_CLKI		(1 << 22)
#define MODE_A_FSI		(1 << 20)
#define MODE_A_FLEN__SHIFT	10
#define MODE_A_FSLEN__SHIFT	0

#define TXC_A_CH1WEX		(1 << 31)
#define TXC_A_CH1EN		(1 << 30)
#define TXC_A_CH1POS__SHIFT	20
#define TXC_A_CH1WID__SHIFT	16
#define TXC_A_CH2WEX		(1 << 15)
#define TXC_A_CH2EN		(1 << 14)
#define TXC_A_CH2POS__SHIFT	4
#define TXC_A_CH2WID__SHIFT	0

//
// DMA controller
//
#define ARM_DMACHAN_CS(chan)		(ARM_DMA_BASE + ((chan) * 0x100) + 0x00)
	#define CS_RESET			(1 << 31)
	#define CS_ABORT			(1 << 30)
	#define CS_WAIT_FOR_OUTSTANDING_WRITES	(1 << 28)
	#define CS_PANIC_PRIORITY_SHIFT		20
		#define DEFAULT_PANIC_PRIORITY		15
	#define CS_PRIORITY_SHIFT		16
		#define DEFAULT_PRIORITY		1
	#define CS_ERROR			(1 << 8)
	#define CS_INT				(1 << 2)
	#define CS_END				(1 << 1)
	#define CS_ACTIVE			(1 << 0)
#define ARM_DMACHAN_CONBLK_AD(chan)	(ARM_DMA_BASE + ((chan) * 0x100) + 0x04)
#define ARM_DMACHAN_TI(chan)		(ARM_DMA_BASE + ((chan) * 0x100) + 0x08)
	#define TI_PERMAP_SHIFT			16
	#define TI_BURST_LENGTH_SHIFT		12
		#define DEFAULT_BURST_LENGTH		0
	#define TI_SRC_IGNORE			(1 << 11)
	#define TI_SRC_DREQ			(1 << 10)
	#define TI_SRC_WIDTH			(1 << 9)
	#define TI_SRC_INC			(1 << 8)
	#define TI_DEST_DREQ			(1 << 6)
	#define TI_DEST_WIDTH			(1 << 5)
	#define TI_DEST_INC			(1 << 4)
	#define TI_WAIT_RESP			(1 << 3)
	#define TI_TDMODE			(1 << 1)
	#define TI_INTEN			(1 << 0)
#define ARM_DMACHAN_SOURCE_AD(chan)	(ARM_DMA_BASE + ((chan) * 0x100) + 0x0C)
#define ARM_DMACHAN_DEST_AD(chan)	(ARM_DMA_BASE + ((chan) * 0x100) + 0x10)
#define ARM_DMACHAN_TXFR_LEN(chan)	(ARM_DMA_BASE + ((chan) * 0x100) + 0x14)
	#define TXFR_LEN_XLENGTH_SHIFT		0
	#define TXFR_LEN_YLENGTH_SHIFT		16
	#define TXFR_LEN_MAX			0x3FFFFFFF
#define ARM_DMACHAN_STRIDE(chan)	(ARM_DMA_BASE + ((chan) * 0x100) + 0x18)
	#define STRIDE_SRC_SHIFT		0
	#define STRIDE_DEST_SHIFT		16
#define ARM_DMACHAN_NEXTCONBK(chan)	(ARM_DMA_BASE + ((chan) * 0x100) + 0x1C)
#define ARM_DMACHAN_DEBUG(chan)		(ARM_DMA_BASE + ((chan) * 0x100) + 0x20)
	#define DEBUG_LITE			(1 << 28)
#define ARM_DMA_INT_STATUS		(ARM_DMA_BASE + 0xFE0)
#define ARM_DMA_ENABLE			(ARM_DMA_BASE + 0xFF0)

CI2SSoundBaseDevice::CI2SSoundBaseDevice (CInterruptSystem *pInterrupt,
					  unsigned	    nSampleRate,
					  unsigned	    nChunkSize)
:	CSoundBaseDevice (SoundFormatSigned24, 0, nSampleRate),
	m_pInterruptSystem (pInterrupt),
	m_nChunkSize (nChunkSize),
	m_PCMCLKPin (18, GPIOModeAlternateFunction0),
	m_PCMFSPin (19, GPIOModeAlternateFunction0),
	m_PCMDOUTPin (21, GPIOModeAlternateFunction0),
	m_Clock (GPIOClockPCM, GPIOClockSourcePLLD),
#define CLOCK_FREQ	500000000
	m_bIRQConnected (FALSE),
	m_State (I2SSoundIdle)
{
	assert (m_pInterruptSystem != 0);
	assert (m_nChunkSize > 0);
	assert ((m_nChunkSize & 1) == 0);

	// setup and concatenate DMA buffers and control blocks
	SetupDMAControlBlock (0);
	SetupDMAControlBlock (1);
	m_pControlBlock[0]->nNextControlBlockAddress = BUS_ADDRESS ((u32) m_pControlBlock[1]);
	m_pControlBlock[1]->nNextControlBlockAddress = BUS_ADDRESS ((u32) m_pControlBlock[0]);

	// start clock and I2S device
	assert (8000 <= nSampleRate && nSampleRate <= 192000);
	assert (CLOCK_FREQ % (CHANLEN*CHANS) == 0);
	unsigned nDivI = CLOCK_FREQ / (CHANLEN*CHANS) / nSampleRate;
	unsigned nTemp = CLOCK_FREQ / (CHANLEN*CHANS) % nSampleRate;
	unsigned nDivF = (nTemp * 4096 + nSampleRate/2) / nSampleRate;
	assert (nDivF <= 4096);
	if (nDivF > 4095)
	{
		nDivI++;
		nDivF = 0;
	}

	m_Clock.Start (nDivI, nDivF, nDivF > 0 ? 1 : 0);

	RunI2S ();

	// enable and reset DMA channel
	PeripheralEntry ();

	assert (!(read32 (ARM_DMACHAN_DEBUG (DMA_CHANNEL_PCM)) & DEBUG_LITE));

	write32 (ARM_DMA_ENABLE, read32 (ARM_DMA_ENABLE) | (1 << DMA_CHANNEL_PCM));
	CTimer::Get ()->usDelay (1000);

	write32 (ARM_DMACHAN_CS (DMA_CHANNEL_PCM), CS_RESET);
	while (read32 (ARM_DMACHAN_CS (DMA_CHANNEL_PCM)) & CS_RESET)
	{
		// do nothing
	}

	PeripheralExit ();

	CDeviceNameService::Get ()->AddDevice ("sndi2s", this, FALSE);
}

CI2SSoundBaseDevice::~CI2SSoundBaseDevice (void)
{
	assert (m_State == I2SSoundIdle);

	// stop I2S device and clock
	StopI2S ();

	// disconnect IRQ
	assert (m_pInterruptSystem != 0);
	if (m_bIRQConnected)
	{
		m_pInterruptSystem->DisconnectIRQ (ARM_IRQ_DMA0+DMA_CHANNEL_PCM);
	}

	m_pInterruptSystem = 0;

	// free buffers
	m_pControlBlock[0] = 0;
	m_pControlBlock[1] = 0;
	delete [] m_pControlBlockBuffer[0];
	m_pControlBlockBuffer[0] = 0;
	delete [] m_pControlBlockBuffer[1];
	m_pControlBlockBuffer[1] = 0;

	delete [] m_pDMABuffer[0];
	m_pDMABuffer[0] = 0;
	delete [] m_pDMABuffer[1];
	m_pDMABuffer[1] = 0;
}

int CI2SSoundBaseDevice::GetRangeMin (void) const
{
	return -(1 << 23)+1;
}

int CI2SSoundBaseDevice::GetRangeMax (void) const
{
	return (1 << 23)-1;
}

boolean CI2SSoundBaseDevice::Start (void)
{
	assert (m_State == I2SSoundIdle);

	// fill buffer 0
	m_nNextBuffer = 0;

	if (!GetNextChunk ())
	{
		return FALSE;
	}

	m_State = I2SSoundRunning;

	// connect IRQ
	if (!m_bIRQConnected)
	{
		assert (m_pInterruptSystem != 0);
		m_pInterruptSystem->ConnectIRQ (ARM_IRQ_DMA0+DMA_CHANNEL_PCM, InterruptStub, this);

		m_bIRQConnected = TRUE;
	}

	// enable I2S DMA operation
	PeripheralEntry ();
	write32 (ARM_PCM_CS_A, read32 (ARM_PCM_CS_A) | CS_A_DMAEN);
	PeripheralExit ();

	// start DMA
	PeripheralEntry ();

	assert (!(read32 (ARM_DMACHAN_CS (DMA_CHANNEL_PCM)) & CS_INT));
	assert (!(read32 (ARM_DMA_INT_STATUS) & (1 << DMA_CHANNEL_PCM)));

	assert (m_pControlBlock[0] != 0);
	write32 (ARM_DMACHAN_CONBLK_AD (DMA_CHANNEL_PCM), BUS_ADDRESS ((u32) m_pControlBlock[0]));

	write32 (ARM_DMACHAN_CS (DMA_CHANNEL_PCM),   CS_WAIT_FOR_OUTSTANDING_WRITES
					           | (DEFAULT_PANIC_PRIORITY << CS_PANIC_PRIORITY_SHIFT)
					           | (DEFAULT_PRIORITY << CS_PRIORITY_SHIFT)
					           | CS_ACTIVE);

	PeripheralExit ();

	// fill buffer 1
	if (!GetNextChunk ())
	{
		m_SpinLock.Acquire ();

		if (m_State == I2SSoundRunning)
		{
			PeripheralEntry ();
			write32 (ARM_DMACHAN_NEXTCONBK (DMA_CHANNEL_PCM), 0);
			PeripheralExit ();

			m_State = I2SSoundTerminating;
		}

		m_SpinLock.Release ();
	}

	return TRUE;
}

void CI2SSoundBaseDevice::Cancel (void)
{
	m_SpinLock.Acquire ();

	if (m_State == I2SSoundRunning)
	{
		m_State = I2SSoundCancelled;
	}

	m_SpinLock.Release ();
}

boolean CI2SSoundBaseDevice::IsActive (void) const
{
	return m_State != I2SSoundIdle ? TRUE : FALSE;
}

boolean CI2SSoundBaseDevice::GetNextChunk (void)
{
	assert (m_pDMABuffer[m_nNextBuffer] != 0);
	unsigned nChunkSize = GetChunk (m_pDMABuffer[m_nNextBuffer], m_nChunkSize);
	if (nChunkSize == 0)
	{
		return FALSE;
	}

	unsigned nTransferLength = nChunkSize * sizeof (u32);
	assert (nTransferLength <= TXFR_LEN_MAX);

	assert (m_pControlBlock[m_nNextBuffer] != 0);
	m_pControlBlock[m_nNextBuffer]->nTransferLength = nTransferLength;

	CleanAndInvalidateDataCacheRange ((u32) m_pDMABuffer[m_nNextBuffer], nTransferLength);
	CleanAndInvalidateDataCacheRange ((u32) m_pControlBlock[m_nNextBuffer], sizeof (TDMAControlBlock));

	m_nNextBuffer ^= 1;

	return TRUE;
}

void CI2SSoundBaseDevice::RunI2S (void)
{
	PeripheralEntry ();

	// disable I2S
	write32 (ARM_PCM_CS_A, 0);
	CTimer::Get ()->usDelay (10);

	// clearing FIFOs
	write32 (ARM_PCM_CS_A, read32 (ARM_PCM_CS_A) | CS_A_TXCLR | CS_A_RXCLR);
	CTimer::Get ()->usDelay (10);

	// enable channel 1 and 2
	write32 (ARM_PCM_TXC_A,   TXC_A_CH1WEX
				| TXC_A_CH1EN
				| (1 << TXC_A_CH1POS__SHIFT)
				| (0 << TXC_A_CH1WID__SHIFT)
				| TXC_A_CH2WEX
				| TXC_A_CH2EN
				| ((CHANLEN+1) << TXC_A_CH2POS__SHIFT)
				| (0 << TXC_A_CH2WID__SHIFT));
	write32 (ARM_PCM_MODE_A,   MODE_A_CLKI
				 | MODE_A_FSI
				 | ((CHANS*CHANLEN-1) << MODE_A_FLEN__SHIFT)
				 | (CHANLEN << MODE_A_FSLEN__SHIFT));

	// disable standby
	write32 (ARM_PCM_CS_A, read32 (ARM_PCM_CS_A) | CS_A_STBY);
	CTimer::Get ()->usDelay (50);

	// enable I2S
	write32 (ARM_PCM_CS_A, read32 (ARM_PCM_CS_A) | CS_A_EN);
	CTimer::Get ()->usDelay (10);

	// enable TX
	write32 (ARM_PCM_CS_A, read32 (ARM_PCM_CS_A) | CS_A_TXON);
	CTimer::Get ()->usDelay (10);

	PeripheralExit ();
}

void CI2SSoundBaseDevice::StopI2S (void)
{
	PeripheralEntry ();

	write32 (ARM_PCM_CS_A, 0);
	CTimer::Get ()->usDelay (50);

	PeripheralExit ();

	m_Clock.Stop ();
}

void CI2SSoundBaseDevice::InterruptHandler (void)
{
	assert (m_State != I2SSoundIdle);

	PeripheralEntry ();

#ifndef NDEBUG
	u32 nIntStatus = read32 (ARM_DMA_INT_STATUS);
#endif
	u32 nIntMask = 1 << DMA_CHANNEL_PCM;
	assert (nIntStatus & nIntMask);
	write32 (ARM_DMA_INT_STATUS, nIntMask);

	u32 nCS = read32 (ARM_DMACHAN_CS (DMA_CHANNEL_PCM));
	assert (nCS & CS_INT);
	write32 (ARM_DMACHAN_CS (DMA_CHANNEL_PCM), nCS);	// reset CS_INT

	PeripheralExit ();

	if (nCS & CS_ERROR)
	{
		m_State = I2SSoundError;

		return;
	}

	m_SpinLock.Acquire ();

	switch (m_State)
	{
	case I2SSoundRunning:
		if (GetNextChunk ())
		{
			break;
		}
		// fall through

	case I2SSoundCancelled:
		PeripheralEntry ();
		write32 (ARM_DMACHAN_NEXTCONBK (DMA_CHANNEL_PCM), 0);
		PeripheralExit ();

		m_State = I2SSoundTerminating;
		break;

	case I2SSoundTerminating:
		m_State = I2SSoundIdle;
		break;

	default:
		assert (0);
		break;
	}

	m_SpinLock.Release ();
}

void CI2SSoundBaseDevice::InterruptStub (void *pParam)
{
	CI2SSoundBaseDevice *pThis = (CI2SSoundBaseDevice *) pParam;
	assert (pThis != 0);

	pThis->InterruptHandler ();
}

void CI2SSoundBaseDevice::SetupDMAControlBlock (unsigned nID)
{
	assert (nID <= 1);

	m_pDMABuffer[nID] = new u32[m_nChunkSize];
	assert (m_pDMABuffer[nID] != 0);

	m_pControlBlockBuffer[nID] = new u8[sizeof (TDMAControlBlock) + 31];
	assert (m_pControlBlockBuffer[nID] != 0);
	m_pControlBlock[nID] = (TDMAControlBlock *) (((u32) m_pControlBlockBuffer[nID] + 31) & ~31);

	m_pControlBlock[nID]->nTransferInformation     =   (DREQSourcePCMTX << TI_PERMAP_SHIFT)
						         | (DEFAULT_BURST_LENGTH << TI_BURST_LENGTH_SHIFT)
						         | TI_SRC_WIDTH
						         | TI_SRC_INC
						         | TI_DEST_DREQ
						         | TI_WAIT_RESP
						         | TI_INTEN;
	m_pControlBlock[nID]->nSourceAddress           = BUS_ADDRESS ((u32) m_pDMABuffer[nID]);
	m_pControlBlock[nID]->nDestinationAddress      = (ARM_PCM_FIFO_A & 0xFFFFFF) + GPU_IO_BASE;
	m_pControlBlock[nID]->n2DModeStride            = 0;
	m_pControlBlock[nID]->nReserved[0]	       = 0;
	m_pControlBlock[nID]->nReserved[1]	       = 0;
}
