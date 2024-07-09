//
// i2ssoundbasedevice-rp1.cpp
//
// Supports:
//	RP1 I2S output and input (not at once)
//	two (or eight, TX master only) 24-bit audio channels
//	cyclic DMA operation or programmed I/O (two channels only) operation with interrupt
//	output tested with PCM5102A and PCM5122 DAC
//
// Ported from the Linux driver:
//	sound/soc/dwc/dwc-i2s.c
//	ALSA SoC Synopsys I2S Audio Layer
//	Copyright (C) 2010 ST Microelectronics
//	Rajeev Kumar <rajeevkumar.linux@gmail.com>
//	Licensed under GPLv2
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2024  R. Stange <rsta2@o2online.de>
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
#include <circle/sound/i2ssoundbasedevice.h>
#include <circle/devicenameservice.h>
#include <circle/bcm2712.h>
#include <circle/memio.h>
#include <circle/rp1int.h>
#include <circle/macros.h>
#include <circle/util.h>
#include <assert.h>

#define CHANS		2			// 2 I2S stereo channels (per stream)
#define CHANLEN		32			// width of a channel slot in bits

#define DMA_CHANNEL	0			// TODO: allocate dynamically

/* common register for all channel */
#define IER		0x000
#define IRER		0x004
#define ITER		0x008
#define CER		0x00C
#define CCR		0x010
#define RXFFR		0x014
#define TXFFR		0x018

/* Enable register fields */
#define IER_TDM_SLOTS_SHIFT 8
#define IER_FRAME_OFF_SHIFT 5
#define IER_FRAME_OFF	BIT(5)
#define IER_INTF_TYPE	BIT(1)
#define IER_IEN		BIT(0)

/* Interrupt status register fields */
#define ISR_TXFO	BIT(5)
#define ISR_TXFE	BIT(4)
#define ISR_RXFO	BIT(1)
#define ISR_RXDA	BIT(0)

/* I2STxRxRegisters for all channels */
#define LRBR_LTHR(x)	(0x40 * x + 0x020)
#define RRBR_RTHR(x)	(0x40 * x + 0x024)
#define RER(x)		(0x40 * x + 0x028)
#define TER(x)		(0x40 * x + 0x02C)
#define RCR(x)		(0x40 * x + 0x030)
#define TCR(x)		(0x40 * x + 0x034)
	#define xCR_FORMAT_S16_LE	0x02
	#define xCR_FORMAT_S24_LE	0x04
	#define xCR_FORMAT_S32_LE	0x05
#define ISR(x)		(0x40 * x + 0x038)
#define IMR(x)		(0x40 * x + 0x03C)
#define ROR(x)		(0x40 * x + 0x040)
#define TOR(x)		(0x40 * x + 0x044)
#define RFCR(x)		(0x40 * x + 0x048)
#define TFCR(x)		(0x40 * x + 0x04C)
#define RFF(x)		(0x40 * x + 0x050)
#define TFF(x)		(0x40 * x + 0x054)
#define RSLOT_TSLOT(x)	(0x4 * (x) + 0x224)

/* Receive enable register fields */
#define RER_RXSLOT_SHIFT 8
#define RER_RXCHEN	BIT(0)

/* Transmit enable register fields */
#define TER_TXSLOT_SHIFT 8
#define TER_TXCHEN	BIT(0)

#define DMACR_DMAEN_TX		BIT(17)
#define DMACR_DMAEN_RX		BIT(16)
#define DMACR_DMAEN_TXCH3	BIT(11)
#define DMACR_DMAEN_TXCH2	BIT(10)
#define DMACR_DMAEN_TXCH1	BIT(9)
#define DMACR_DMAEN_TXCH0	BIT(8)
#define DMACR_DMAEN_RXCH3	BIT(3)
#define DMACR_DMAEN_RXCH2	BIT(2)
#define DMACR_DMAEN_RXCH1	BIT(1)
#define DMACR_DMAEN_RXCH0	BIT(0)

/* I2SCOMPRegisters */
#define I2S_COMP_PARAM_2	0x01F0
#define I2S_COMP_PARAM_1	0x01F4
#define I2S_COMP_VERSION	0x01F8
#define I2S_COMP_TYPE		0x01FC

#define I2S_RXDMA		0x01C0
#define I2S_RRXDMA		0x01C4
#define I2S_TXDMA		0x01C8
#define I2S_RTXDMA		0x01CC
#define I2S_DMACR		0x0200
#define I2S_DMAEN_RXBLOCK	(1 << 16)
#define I2S_DMAEN_TXBLOCK	(1 << 17)

/*
 * Component parameter register fields - define the I2S block's
 * configuration.
 */
#define GENMASK(h, l)		(((~0UL) - (1UL << (l)) + 1) &	\
				 (~0UL >> (64 - 1 - (h))))

#define	COMP1_TX_WORDSIZE_3(r)	(((r) & GENMASK(27, 25)) >> 25)
#define	COMP1_TX_WORDSIZE_2(r)	(((r) & GENMASK(24, 22)) >> 22)
#define	COMP1_TX_WORDSIZE_1(r)	(((r) & GENMASK(21, 19)) >> 19)
#define	COMP1_TX_WORDSIZE_0(r)	(((r) & GENMASK(18, 16)) >> 16)
#define	COMP1_TX_CHANNELS(r)	(((r) & GENMASK(10, 9)) >> 9)
#define	COMP1_RX_CHANNELS(r)	(((r) & GENMASK(8, 7)) >> 7)
#define	COMP1_RX_ENABLED(r)	(((r) & BIT(6)) >> 6)
#define	COMP1_TX_ENABLED(r)	(((r) & BIT(5)) >> 5)
#define	COMP1_MODE_EN(r)	(((r) & BIT(4)) >> 4)
#define	COMP1_FIFO_DEPTH_GLOBAL(r)	(((r) & GENMASK(3, 2)) >> 2)
#define	COMP1_APB_DATA_WIDTH(r)	(((r) & GENMASK(1, 0)) >> 0)

#define	COMP2_RX_WORDSIZE_3(r)	(((r) & GENMASK(12, 10)) >> 10)
#define	COMP2_RX_WORDSIZE_2(r)	(((r) & GENMASK(9, 7)) >> 7)
#define	COMP2_RX_WORDSIZE_1(r)	(((r) & GENMASK(5, 3)) >> 3)
#define	COMP2_RX_WORDSIZE_0(r)	(((r) & GENMASK(2, 0)) >> 0)

CI2SSoundBaseDevice::CI2SSoundBaseDevice (CInterruptSystem *pInterrupt,
					  unsigned	    nSampleRate,
					  unsigned	    nChunkSize,
					  bool		    bSlave,
					  CI2CMaster       *pI2CMaster,
					  u8                ucI2CAddress,
					  TDeviceMode       DeviceMode,
					  unsigned	    nHWChannels)
:
#ifdef USE_I2S_SOUND_IRQ
	m_pInterruptSystem (pInterrupt),
#else
	m_nChunkSize (nChunkSize),
#endif
	m_nSampleRate (nSampleRate),
	m_bSlave (bSlave),
	m_pI2CMaster (pI2CMaster),
	m_ucI2CAddress (ucI2CAddress),
	m_DeviceMode (DeviceMode),
	m_nHWChannels (nHWChannels),
	m_PCMCLKPin (18, m_bSlave ? GPIOModeAlternateFunction4 : GPIOModeAlternateFunction2),
	m_PCMFSPin (19, m_bSlave ? GPIOModeAlternateFunction4 : GPIOModeAlternateFunction2),
	m_PCMDINPin (20, m_bSlave ? GPIOModeAlternateFunction4 : GPIOModeAlternateFunction2),
	m_PCMDOUTPin (21, m_bSlave ? GPIOModeAlternateFunction4 : GPIOModeAlternateFunction2),
	m_Clock (GPIOClockI2S, GPIOClockSourcePLLAudio),
	m_ulBase (m_bSlave ? ARM_I2S1_BASE : ARM_I2S0_BASE),
#ifdef USE_I2S_SOUND_IRQ
	m_bInterruptConnected (FALSE),
#endif
	m_State (StateIdle),
#ifndef USE_I2S_SOUND_IRQ
	m_DMAChannel (DMA_CHANNEL, pInterrupt),
	m_pDMABuffer {new u32[m_nChunkSize], new u32[m_nChunkSize]},
#endif
	m_bControllerInited (FALSE),
	m_pController (nullptr)
{
	Setup (SoundFormatSigned24_32, 0, nSampleRate, m_nHWChannels, m_nHWChannels, FALSE);

	if (m_DeviceMode == DeviceModeTXRX)
	{
		m_State = StateFailed;

		return;
	}

	assert (m_nHWChannels == 2 || m_nHWChannels == 8);
	if (m_nHWChannels == 8)
	{
#ifdef USE_I2S_SOUND_IRQ
		assert (0);	// 8 channels TX does not work in IRQ mode!
#endif
		assert (m_DeviceMode == DeviceModeTXOnly);
		assert (!m_bSlave);

		m_PCMDOUTPin1.AssignPin (23);
		m_PCMDOUTPin1.SetMode (GPIOModeAlternateFunction2);
		m_PCMDOUTPin2.AssignPin (25);
		m_PCMDOUTPin2.SetMode (GPIOModeAlternateFunction2);
		m_PCMDOUTPin3.AssignPin (27);
		m_PCMDOUTPin3.SetMode (GPIOModeAlternateFunction2);
	}

	CDeviceNameService::Get ()->AddDevice ("sndi2s", this, FALSE);
}

CI2SSoundBaseDevice::~CI2SSoundBaseDevice (void)
{
	assert (   m_State != StateRunning
		&& m_State != StateCanceled);

	CDeviceNameService::Get ()->RemoveDevice ("sndi2s", FALSE);

	delete m_pController;
	m_pController = nullptr;

	StopI2S ();

#ifndef USE_I2S_SOUND_IRQ
	delete [] m_pDMABuffer[0];
	delete [] m_pDMABuffer[1];
#endif
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
	if (m_State == StateFailed)
	{
		return FALSE;
	}

	if (!m_bControllerInited)
	{
		if (!ControllerFactory ())
		{
			m_State = StateFailed;

			return FALSE;
		}

		m_bControllerInited = TRUE;
	}

	m_State = StateRunning;

#ifndef USE_I2S_SOUND_IRQ
	assert (m_pDMABuffer[0]);
	assert (m_pDMABuffer[1]);
	size_t ulBufferSize = m_nChunkSize * sizeof (u32);

	if (m_DeviceMode == DeviceModeTXOnly)
	{
		memset (m_pDMABuffer[0], 0, ulBufferSize);
		memset (m_pDMABuffer[1], 0, ulBufferSize);

		const void *Buffers[2] = {m_pDMABuffer[0], m_pDMABuffer[1]};
		m_DMAChannel.SetupCyclicIOWrite (m_ulBase + I2S_TXDMA,
						 Buffers, 2, ulBufferSize,
						 m_bSlave ? CDMAChannelRP1::DREQSourceI2S1TX
							  : CDMAChannelRP1::DREQSourceI2S0TX);
	}
	else
	{
		assert (m_DeviceMode == DeviceModeRXOnly);

		void *Buffers[2] = {m_pDMABuffer[0], m_pDMABuffer[1]};
		m_DMAChannel.SetupCyclicIORead (Buffers,
						m_ulBase + I2S_RXDMA, 2, ulBufferSize,
						m_bSlave ? CDMAChannelRP1::DREQSourceI2S1RX
							 : CDMAChannelRP1::DREQSourceI2S0RX);
	}

	m_DMAChannel.SetCompletionRoutine (DMACompletionStub, this);
	m_DMAChannel.Start ();
#endif

	if (!RunI2S ())
	{
#ifndef USE_I2S_SOUND_IRQ
		m_DMAChannel.Cancel ();
#endif

		m_State = StateFailed;

		return FALSE;
	}

	return TRUE;
}

void CI2SSoundBaseDevice::Cancel (void)
{
#ifdef USE_I2S_SOUND_IRQ
	StopI2S ();

	m_State = StateIdle;
#else
	m_SpinLock.Acquire ();

	if (m_State == StateRunning)
	{
		m_State = StateCanceled;

		m_SpinLock.Release ();

		m_DMAChannel.Cancel ();
		StopI2S ();

		m_SpinLock.Acquire ();

		m_State = StateIdle;
	}

	m_SpinLock.Release ();
#endif
}

boolean CI2SSoundBaseDevice::IsActive (void) const
{
	TState State = m_State;

	return    State == StateRunning
	       || State == StateCanceled;
}

CSoundController *CI2SSoundBaseDevice::GetController (void)
{
	return m_pController;
}

boolean CI2SSoundBaseDevice::RunI2S (void)
{
	// check device configuration
	u32 nCOMP1 = read32 (m_ulBase + I2S_COMP_PARAM_1);

	// respective mode enabled (master or slave)?
	assert (m_bSlave ? !COMP1_MODE_EN (nCOMP1) : COMP1_MODE_EN (nCOMP1));

	u32 nFIFODepth = 1 << (1 + COMP1_FIFO_DEPTH_GLOBAL (nCOMP1));
	m_nFIFOThreshold = nFIFODepth / 2;

	// start clock
	if (!m_bSlave)
	{
#if CHANS * CHANLEN == 48
		write32 (m_ulBase + CCR, 0x08);
#elif CHANS * CHANLEN == 64
		write32 (m_ulBase + CCR, 0x10);
#else
	#error CHANS * CHANLEN must be 48 or 64!
#endif

		if (!m_Clock.StartRate (CHANS * CHANLEN * m_nSampleRate))
		{
			return FALSE;
		}
	}

#ifdef USE_I2S_SOUND_IRQ
	// connect interrupt
	assert (!m_bInterruptConnected);
	assert (m_pInterruptSystem);
	m_pInterruptSystem->ConnectIRQ (m_bSlave ? RP1_IRQ_I2S1 : RP1_IRQ_I2S0, InterruptStub, this);
	m_bInterruptConnected = TRUE;
#endif

	// disable all channels
	for (int i = 0; i < 4; i++)
	{
		write32 (m_ulBase + TER (i), 0);
		write32 (m_ulBase + RER (i), 0);
	}

	// set parameters and enable interrupts
	u32 nDMACR = read32 (m_ulBase + I2S_DMACR);

	if (m_DeviceMode == DeviceModeTXOnly)
	{
		assert (COMP1_TX_ENABLED (nCOMP1));
		assert (2 * (COMP1_TX_CHANNELS (nCOMP1) + 1) >= m_nHWChannels);

		nDMACR &= ~(DMACR_DMAEN_TXCH0 * 0xf);

		for (unsigned i = 0; i < m_nHWChannels / 2; i++)
		{
			write32 (m_ulBase + TCR (i), xCR_FORMAT_S24_LE);
			write32 (m_ulBase + TFCR (i), m_nFIFOThreshold - 1);
			write32 (m_ulBase + TER (i), TER_TXCHEN);

			nDMACR |= DMACR_DMAEN_TXCH0 << i;
		}

		nDMACR |= DMACR_DMAEN_TX;
		write32 (m_ulBase + I2S_DMACR, nDMACR);

		write32 (m_ulBase + TXFFR, 1);
		write32 (m_ulBase + IER, IER_IEN);
		write32 (m_ulBase + ITER, 1);

#ifdef USE_I2S_SOUND_IRQ
		write32 (m_ulBase + IMR (0), read32 (m_ulBase + IMR (0)) & ~0x30);
#else
		write32 (m_ulBase + I2S_DMACR, read32 (m_ulBase + I2S_DMACR) | I2S_DMAEN_TXBLOCK);
#endif
	}
	else
	{
		assert (m_DeviceMode == DeviceModeRXOnly);
		assert (COMP1_RX_ENABLED (nCOMP1));
		assert (m_nHWChannels == 2);

		write32 (m_ulBase + RCR (0), xCR_FORMAT_S24_LE);
		write32 (m_ulBase + RFCR (0), m_nFIFOThreshold - 1);
		write32 (m_ulBase + RER (0), RER_RXCHEN);

		nDMACR &= ~(DMACR_DMAEN_RXCH0 * 0xf);
		nDMACR |= DMACR_DMAEN_RXCH0 | DMACR_DMAEN_RX;
		write32 (m_ulBase + I2S_DMACR, nDMACR);

		write32 (m_ulBase + RXFFR, 1);
		write32 (m_ulBase + IER, IER_IEN);
		write32 (m_ulBase + IRER, 1);

#ifdef USE_I2S_SOUND_IRQ
		write32 (m_ulBase + IMR (0), read32 (m_ulBase + IMR (0)) & ~0x03);
#else
		write32 (m_ulBase + I2S_DMACR, read32 (m_ulBase + I2S_DMACR) | I2S_DMAEN_RXBLOCK);
#endif
	}

	write32 (m_ulBase + CER, 1);

	return TRUE;
}

void CI2SSoundBaseDevice::StopI2S (void)
{
#ifdef USE_I2S_SOUND_IRQ
	if (m_bInterruptConnected)
	{
		m_pInterruptSystem->DisconnectIRQ (m_bSlave ? RP1_IRQ_I2S1 : RP1_IRQ_I2S0);

		m_bInterruptConnected = FALSE;
	}
#endif

	for (int i = 0; i < 4; i++)
	{
#ifdef USE_I2S_SOUND_IRQ
		// clear interrupts
		read32 (m_ulBase + TOR (i));
		read32 (m_ulBase + ROR (i));

		// disable interrupts
		write32 (m_ulBase + IMR (i), read32 (m_ulBase + IMR (i)) | 0x30 | 0x03);
#endif

		// disable channels
		write32 (m_ulBase + TER (i), 0);
		write32 (m_ulBase + RER (i), 0);
	}

	// shutdown device
	write32 (m_ulBase + ITER, 0);
	write32 (m_ulBase + IRER, 0);
	write32 (m_ulBase + CER, 0);
	write32 (m_ulBase + IER, 0);

	if (!m_bSlave)
	{
		m_Clock.Stop ();
	}
}

#ifdef USE_I2S_SOUND_IRQ

void CI2SSoundBaseDevice::InterruptHandler (void)
{
	u32 nStatus = read32 (m_ulBase + ISR (0)) & ~read32 (m_ulBase + IMR (0));

	// clear interrupts
	read32 (m_ulBase + TOR (0));
	read32 (m_ulBase + ROR (0));

	if (nStatus & ISR_TXFE)
	{
		const unsigned nChunkSize = CHANS * m_nFIFOThreshold;
		u32 Buffer[nChunkSize];

		if (GetChunk (Buffer, nChunkSize) < nChunkSize)
		{
			m_State = StateIdle;

			// disable TX interrupts
			write32 (m_ulBase + IMR (0), read32 (m_ulBase + IMR (0)) | 0x30);

			return;
		}

		for (unsigned i = 0; i < nChunkSize; i += CHANS)
		{
			write32 (m_ulBase + LRBR_LTHR (0), Buffer[i]);
			write32 (m_ulBase + RRBR_RTHR (0), Buffer[i + 1]);
		}
	}

	if (nStatus & ISR_RXDA)
	{
		const unsigned nChunkSize = CHANS * m_nFIFOThreshold;
		u32 Buffer[nChunkSize];

		for (unsigned i = 0; i < nChunkSize; i += CHANS)
		{
			Buffer[i] = read32 (m_ulBase + LRBR_LTHR (0));
			Buffer[i + 1] = read32 (m_ulBase + RRBR_RTHR (0));
		}

		PutChunk (Buffer, nChunkSize);
	}

	//assert (!(nStatus & ISR_TXFO));
	//assert (!(nStatus & ISR_RXFO));
}

void CI2SSoundBaseDevice::InterruptStub (void *pParam)
{
	CI2SSoundBaseDevice *pThis = static_cast<CI2SSoundBaseDevice *> (pParam);
	assert (pThis);

	pThis->InterruptHandler ();
}

#else	// #ifdef USE_I2S_SOUND_IRQ

void CI2SSoundBaseDevice::DMACompletionRoutine (unsigned nChannel, unsigned nBuffer, boolean bStatus)
{
	m_SpinLock.Acquire ();

	if (m_State != StateRunning)
	{
		m_SpinLock.Release ();

		return;
	}

	if (!bStatus)
	{
		m_State = StateFailed;

		m_SpinLock.Release ();

		m_DMAChannel.Cancel ();
		StopI2S ();

		return;
	}

	assert (nBuffer < 2);
	assert (m_pDMABuffer[nBuffer]);
	assert (m_nChunkSize);

	if (m_DeviceMode == DeviceModeTXOnly)
	{
		if (GetChunk (m_pDMABuffer[nBuffer], m_nChunkSize) < m_nChunkSize)
		{
			m_DMAChannel.Cancel ();
			StopI2S ();

			m_State = StateIdle;

			m_SpinLock.Release ();

			return;
		}
	}
	else
	{
		assert (m_DeviceMode == DeviceModeRXOnly);

		PutChunk (m_pDMABuffer[nBuffer], m_nChunkSize);
	}

	m_SpinLock.Release ();
}

void CI2SSoundBaseDevice::DMACompletionStub (unsigned nChannel, unsigned nBuffer,
					     boolean bStatus, void *pParam)
{
	CI2SSoundBaseDevice *pThis = static_cast<CI2SSoundBaseDevice *> (pParam);
	assert (pThis);

	pThis->DMACompletionRoutine (nChannel, nBuffer, bStatus);
}

#endif

#include <circle/sound/pcm512xsoundcontroller.h>
#include <circle/sound/wm8960soundcontroller.h>

boolean CI2SSoundBaseDevice::ControllerFactory (void)
{
	if (!m_pI2CMaster)
	{
		return TRUE;
	}

	// PCM512x
	m_pController = new CPCM512xSoundController (m_pI2CMaster, m_ucI2CAddress);
	assert (m_pController);

	if (m_pController->Probe ())
	{
		return TRUE;
	}

	delete m_pController;
	m_pController = nullptr;

	// WM8960
	m_pController = new CWM8960SoundController (m_pI2CMaster, m_ucI2CAddress,
						    m_nSampleRate,
						    m_DeviceMode != DeviceModeRXOnly,
						    m_DeviceMode != DeviceModeTXOnly);
	assert (m_pController);

	if (m_pController->Probe ())
	{
		return TRUE;
	}

	delete m_pController;
	m_pController = nullptr;

	return !m_ucI2CAddress;		// PCM5102A does not need a controller
}
