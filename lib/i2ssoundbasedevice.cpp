//
// i2ssoundbasedevice.cpp
//
// Supports:
//	BCM283x/BCM2711 I2S output
//	two 24-bit audio channels
//	sample rate up to 192 KHz
//	tested with PCM5102A and PCM5122 DACs
//
// References:
//	https://www.raspberrypi.org/forums/viewtopic.php?f=44&t=8496
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2021  R. Stange <rsta2@o2online.de>
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
#include <circle/machineinfo.h>
#include <circle/util.h>
#include <circle/new.h>
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
#define MODE_A_CLKM		(1 << 23)
#define MODE_A_FSI		(1 << 20)
#define MODE_A_FSM		(1 << 21)
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

#define DREQ_A_TX__SHIFT	8
#define DREQ_A_TX__MASK		(0x7F << 8)

CI2SSoundBaseDevice::CI2SSoundBaseDevice (CInterruptSystem *pInterrupt,
					  unsigned	    nSampleRate,
					  unsigned	    nChunkSize,
					  bool		    bSlave,
					  CI2CMaster       *pI2CMaster,
					  u8                ucI2CAddress)
:	CSoundBaseDevice (SoundFormatSigned24, 0, nSampleRate),
	m_pInterruptSystem (pInterrupt),
	m_nChunkSize (nChunkSize),
	m_bSlave (bSlave),
	m_pI2CMaster (pI2CMaster),
	m_ucI2CAddress (ucI2CAddress),
	m_PCMCLKPin (18, GPIOModeAlternateFunction0),
	m_PCMFSPin (19, GPIOModeAlternateFunction0),
	m_PCMDOUTPin (21, GPIOModeAlternateFunction0),
	m_Clock (GPIOClockPCM, GPIOClockSourcePLLD),
	m_bI2CInited (FALSE),
	m_bIRQConnected (FALSE),
	m_State (I2SSoundIdle),
	m_nDMAChannel (CMachineInfo::Get ()->AllocateDMAChannel (DMA_CHANNEL_LITE))
{
	assert (m_pInterruptSystem != 0);
	assert (m_nChunkSize >= 32);
	assert ((m_nChunkSize & 1) == 0);

	// setup and concatenate DMA buffers and control blocks
	SetupDMAControlBlock (0);
	SetupDMAControlBlock (1);
	m_pControlBlock[0]->nNextControlBlockAddress = BUS_ADDRESS ((uintptr) m_pControlBlock[1]);
	m_pControlBlock[1]->nNextControlBlockAddress = BUS_ADDRESS ((uintptr) m_pControlBlock[0]);

	// start clock and I2S device
	unsigned nClockFreq = CMachineInfo::Get ()->GetGPIOClockSourceRate (GPIOClockSourcePLLD);
	assert (nClockFreq > 0);
	assert (8000 <= nSampleRate && nSampleRate <= 192000);
	assert (nClockFreq % (CHANLEN*CHANS) == 0);
	unsigned nDivI = nClockFreq / (CHANLEN*CHANS) / nSampleRate;
	unsigned nTemp = nClockFreq / (CHANLEN*CHANS) % nSampleRate;
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

	assert (m_nDMAChannel <= DMA_CHANNEL_MAX);
	write32 (ARM_DMA_ENABLE, read32 (ARM_DMA_ENABLE) | (1 << m_nDMAChannel));
	CTimer::SimpleusDelay (1000);

	write32 (ARM_DMACHAN_CS (m_nDMAChannel), CS_RESET);
	while (read32 (ARM_DMACHAN_CS (m_nDMAChannel)) & CS_RESET)
	{
		// do nothing
	}

	PeripheralExit ();

	CDeviceNameService::Get ()->AddDevice ("sndi2s", this, FALSE);
}

CI2SSoundBaseDevice::~CI2SSoundBaseDevice (void)
{
	assert (m_State == I2SSoundIdle);

	CDeviceNameService::Get ()->RemoveDevice ("sndi2s", FALSE);

	// stop I2S device and clock
	StopI2S ();

	// reset and disable DMA channel
	PeripheralEntry ();

	assert (m_nDMAChannel <= DMA_CHANNEL_MAX);

	write32 (ARM_DMACHAN_CS (m_nDMAChannel), CS_RESET);
	while (read32 (ARM_DMACHAN_CS (m_nDMAChannel)) & CS_RESET)
	{
		// do nothing
	}

	write32 (ARM_DMA_ENABLE, read32 (ARM_DMA_ENABLE) & ~(1 << m_nDMAChannel));

	PeripheralExit ();

	// disconnect IRQ
	assert (m_pInterruptSystem != 0);
	if (m_bIRQConnected)
	{
		m_pInterruptSystem->DisconnectIRQ (ARM_IRQ_DMA0+m_nDMAChannel);
	}

	m_pInterruptSystem = 0;

	// free DMA channel
	CMachineInfo::Get ()->FreeDMAChannel (m_nDMAChannel);

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

	// optional DAC init via I2C
	if (   m_pI2CMaster != 0
	    && !m_bI2CInited)
	{
		if (m_ucI2CAddress != 0)
		{
			if (!InitPCM51xx (m_ucI2CAddress))	// fixed address, must succeed
			{
				return FALSE;
			}
		}
		else
		{
			if (!InitPCM51xx (0x4C))		// auto probing, ignore failure
			{
				InitPCM51xx (0x4D);
			}
		}

		m_bI2CInited = TRUE;
	}

	// fill buffer 0
	m_nNextBuffer = 0;

	if (!GetNextChunk (TRUE))
	{
		return FALSE;
	}

	m_State = I2SSoundRunning;

	// connect IRQ
	assert (m_nDMAChannel <= DMA_CHANNEL_MAX);

	if (!m_bIRQConnected)
	{
		assert (m_pInterruptSystem != 0);
		m_pInterruptSystem->ConnectIRQ (ARM_IRQ_DMA0+m_nDMAChannel, InterruptStub, this);

		m_bIRQConnected = TRUE;
	}

	// enable I2S DMA operation
	PeripheralEntry ();

	if (m_nChunkSize < 64)
	{
		write32 (ARM_PCM_DREQ_A,   (read32 (ARM_PCM_DREQ_A) & ~DREQ_A_TX__MASK)
					 | (0x18 << DREQ_A_TX__SHIFT));
	}

	write32 (ARM_PCM_CS_A, read32 (ARM_PCM_CS_A) | CS_A_DMAEN);

	PeripheralExit ();

	// start DMA
	PeripheralEntry ();

	assert (!(read32 (ARM_DMACHAN_CS (m_nDMAChannel)) & CS_INT));
	assert (!(read32 (ARM_DMA_INT_STATUS) & (1 << m_nDMAChannel)));

	assert (m_pControlBlock[0] != 0);
	write32 (ARM_DMACHAN_CONBLK_AD (m_nDMAChannel), BUS_ADDRESS ((uintptr) m_pControlBlock[0]));

	write32 (ARM_DMACHAN_CS (m_nDMAChannel),   CS_WAIT_FOR_OUTSTANDING_WRITES
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
			write32 (ARM_DMACHAN_NEXTCONBK (m_nDMAChannel), 0);
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

boolean CI2SSoundBaseDevice::GetNextChunk (boolean bFirstCall)
{
	assert (m_pDMABuffer[m_nNextBuffer] != 0);

	unsigned nChunkSize;
	if (!bFirstCall)
	{
		nChunkSize = GetChunk (m_pDMABuffer[m_nNextBuffer], m_nChunkSize);
		if (nChunkSize == 0)
		{
			return FALSE;
		}
	}
	else
	{
		nChunkSize = m_nChunkSize;
		memset (m_pDMABuffer[m_nNextBuffer], 0, nChunkSize * sizeof (u32));
	}

	unsigned nTransferLength = nChunkSize * sizeof (u32);
	assert (nTransferLength <= TXFR_LEN_MAX_LITE);

	assert (m_pControlBlock[m_nNextBuffer] != 0);
	m_pControlBlock[m_nNextBuffer]->nTransferLength = nTransferLength;

	CleanAndInvalidateDataCacheRange ((uintptr) m_pDMABuffer[m_nNextBuffer], nTransferLength);
	CleanAndInvalidateDataCacheRange ((uintptr) m_pControlBlock[m_nNextBuffer], sizeof (TDMAControlBlock));

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

	u32 nModeA =   MODE_A_CLKI
		     | MODE_A_FSI
		     | ((CHANS*CHANLEN-1) << MODE_A_FLEN__SHIFT)
		     | (CHANLEN << MODE_A_FSLEN__SHIFT);

	// set PCM clock and frame sync as inputs if in slave mode
	if (m_bSlave)
	{
		nModeA |= MODE_A_CLKM | MODE_A_FSM;
	}

	write32 (ARM_PCM_MODE_A, nModeA);

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
	assert (m_nDMAChannel <= DMA_CHANNEL_MAX);

	PeripheralEntry ();

#ifndef NDEBUG
	u32 nIntStatus = read32 (ARM_DMA_INT_STATUS);
#endif
	u32 nIntMask = 1 << m_nDMAChannel;
	assert (nIntStatus & nIntMask);
	write32 (ARM_DMA_INT_STATUS, nIntMask);

	u32 nCS = read32 (ARM_DMACHAN_CS (m_nDMAChannel));
	assert (nCS & CS_INT);
	write32 (ARM_DMACHAN_CS (m_nDMAChannel), nCS);	// reset CS_INT

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
		write32 (ARM_DMACHAN_NEXTCONBK (m_nDMAChannel), 0);
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

	m_pDMABuffer[nID] = new (HEAP_DMA30) u32[m_nChunkSize];
	assert (m_pDMABuffer[nID] != 0);

	m_pControlBlockBuffer[nID] = new (HEAP_DMA30) u8[sizeof (TDMAControlBlock) + 31];
	assert (m_pControlBlockBuffer[nID] != 0);
	m_pControlBlock[nID] = (TDMAControlBlock *) (((uintptr) m_pControlBlockBuffer[nID] + 31) & ~31);

	m_pControlBlock[nID]->nTransferInformation     =   (DREQSourcePCMTX << TI_PERMAP_SHIFT)
						         | (DEFAULT_BURST_LENGTH << TI_BURST_LENGTH_SHIFT)
						         | TI_SRC_WIDTH
						         | TI_SRC_INC
						         | TI_DEST_DREQ
						         | TI_WAIT_RESP
						         | TI_INTEN;
	m_pControlBlock[nID]->nSourceAddress           = BUS_ADDRESS ((uintptr) m_pDMABuffer[nID]);
	m_pControlBlock[nID]->nDestinationAddress      = (ARM_PCM_FIFO_A & 0xFFFFFF) + GPU_IO_BASE;
	m_pControlBlock[nID]->n2DModeStride            = 0;
	m_pControlBlock[nID]->nReserved[0]	       = 0;
	m_pControlBlock[nID]->nReserved[1]	       = 0;
}

//
// Taken from the file mt32pi.cpp from this project:
//
// mt32-pi - A baremetal MIDI synthesizer for Raspberry Pi
// Copyright (C) 2020-2021 Dale Whinham <daleyo@gmail.com>
//
// Licensed under GPLv3
//
boolean CI2SSoundBaseDevice::InitPCM51xx (u8 ucI2CAddress)
{
	static const u8 initBytes[][2] =
	{
		// Set PLL reference clock to BCK (set SREF to 001b)
		{ 0x0d, 0x10 },

		// Ignore clock halt detection (set IDCH to 1)
		{ 0x25, 0x08 },

		// Disable auto mute
		{ 0x41, 0x04 }
	};

	for (auto &command : initBytes)
	{
		if (   m_pI2CMaster->Write (ucI2CAddress, &command, sizeof (command))
		    != sizeof (command))
		{
			return FALSE;
		}
	}

	return TRUE;
}
