//
// hdmisoundbasedevice.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2021-2023  R. Stange <rsta2@o2online.de>
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
#include <circle/sound/hdmisoundbasedevice.h>
#include <circle/devicenameservice.h>
#include <circle/bcm2835.h>
#include <circle/bcm2835int.h>
#include <circle/memio.h>
#include <circle/machineinfo.h>
#include <circle/synchronize.h>
#include <circle/logger.h>
#include <circle/timer.h>
#include <circle/macros.h>
#include <circle/util.h>
#include <circle/new.h>
#include <assert.h>

//#define HDMI_DEBUG

#if RASPPI <= 3
	#define REG(name, base, offset, base4, offset4)	\
					static const u32 Reg##name = (base) + (offset);
#else
	#define REG(name, base, offset, base4, offset4)	\
					static const u32 Reg##name = (base4) + (offset4);
#endif

#define REGBIT(reg, name, bitnr)	static const u32 Bit##reg##name = 1U << (bitnr);
#define REGSHIFT(reg, name, shift)	static const int Shift##reg##name = shift;
#define REGMASK(reg, name, mask)	static const u32 Mask##reg##name = mask;
#define REGVALUE(reg, field, name, val)	static const u32 reg##field##name = val;

#define UNUSED		0

REG (AudioPacketConfig, ARM_HDMI_BASE, 0x9C, ARM_HDMI_BASE, 0xB8);
	REGSHIFT (AudioPacketConfig, CeaMask, 0);
	REGSHIFT (AudioPacketConfig, BFrameIdentifier, 10);
	REGBIT (AudioPacketConfig, ZeroDataOnInactiveChannels, 24);
	REGBIT (AudioPacketConfig, ZeroDataOnSampleFlat, 29);
REG (CrpConfig, ARM_HDMI_BASE, 0xA8, ARM_HDMI_BASE, 0xC8);	// clock recover
	REGBIT (CrpConfig, ExternalCtsEnable, 24);
	REGSHIFT (CrpConfig, ConfigN, 0);
REG (Cts0, ARM_HDMI_BASE, 0xAC, ARM_HDMI_BASE, 0xCC); 		// clock time stamp
REG (Cts1, ARM_HDMI_BASE, 0xB0, ARM_HDMI_BASE, 0xD0);
REG (MaiChannelMap, ARM_HDMI_BASE, 0x90, ARM_HDMI_BASE, 0x9C);
REG (MaiConfig, ARM_HDMI_BASE, 0x94, ARM_HDMI_BASE, 0xA0);
	REGSHIFT (MaiConfig, ChannelMask, 0);
	REGBIT (MaiConfig, BitReverse, 26);
	REGBIT (MaiConfig, FormatReverse, 27);
REG (MaiControl, ARM_HD_BASE, 0x14, ARM_HD_BASE, 0x10);
	REGBIT (MaiControl, Reset, 0);
	REGBIT (MaiControl, ErrorFull, 1);
	REGBIT (MaiControl, ErrorEmpty, 2);
	REGBIT (MaiControl, Enable, 3);
	REGSHIFT (MaiControl, ChannelNumber, 4);
	REGBIT (MaiControl, Flush, 9);
	//REGBIT (MaiControl, Empty, 10);
	REGBIT (MaiControl, Full, 11);
	REGBIT (MaiControl, WholSample, 12);
	REGBIT (MaiControl, ChannelAlign, 13);
	//REGBIT (MaiControl, Busy, 14);
	REGBIT (MaiControl, Delayed, 15);
REG (MaiData, ARM_HD_BASE, 0x20, ARM_HD_BASE, 0x1C);
REG (MaiFormat, ARM_HD_BASE, 0x1C, ARM_HD_BASE, 0x18);
	REGSHIFT (MaiFormat, SampleRate, 8);
		REGVALUE (MaiFormat, SampleRate, NotIndicated, 0);
		REGVALUE (MaiFormat, SampleRate, 8000, 1);
	REGSHIFT (MaiFormat, AudioFormat, 16);
		REGVALUE (MaiFormat, AudioFormat, PCM, 2);
REG (MaiSampleRate, ARM_HD_BASE, 0x2C, ARM_HD_BASE, 0x20);
	REGSHIFT (MaiSampleRate, M, 0);
	REGMASK (MaiSampleRate, M, 0xFFU);
	REGSHIFT (MaiSampleRate, N, 8);
	REGMASK (MaiSampleRate, N, 0xFFFFFF00U);
REG (MaiThreshold, ARM_HD_BASE, 0x18, ARM_HD_BASE, 0x14);
	REGSHIFT (MaiThreshold, DREQLow, 0);
	REGSHIFT (MaiThreshold, DREQHigh, 8);
	REGSHIFT (MaiThreshold, PanicLow, 16);
	REGSHIFT (MaiThreshold, PanicHigh, 24);
		REGVALUE (MaiThreshold, Any, Default, 16);
REG (RamPacketAudio0, ARM_HDMI_BASE, 0x490, ARM_RAM_BASE, 0x90);
REG (RamPacketAudio1, ARM_HDMI_BASE, 0x494, ARM_RAM_BASE, 0x94);
REG (RamPacketAudio2, ARM_HDMI_BASE, 0x498, ARM_RAM_BASE, 0x98);
REG (RamPacketAudio8, ARM_HDMI_BASE, 0x4B0, ARM_RAM_BASE, 0xB0);
REG (RamPacketConfig, ARM_HDMI_BASE, 0xA0, ARM_HDMI_BASE, 0xBC);
	REGBIT (RamPacketConfig, AudioPacketIdentifier, 4);
	REGBIT (RamPacketConfig, Enable, 16);
REG (RamPacketStatus, ARM_HDMI_BASE, 0xA4, ARM_HDMI_BASE, 0xC4);
	REGBIT (RamPacketStatus, AudioPacketIdentifier, 4);
#if RASPPI <= 3
REG (TxPhyControl0, ARM_HDMI_BASE, 0x2C4, UNUSED, UNUSED);
	REGBIT (TxPhyControl0, RngPowerDown, 25);
#else
REG (TxPhyPowerDownControl, UNUSED, UNUSED, ARM_PHY_BASE, 0x04);
	REGBIT (TxPhyPowerDownControl, RngGenPowerDown, 4);
#endif

static const char DeviceName[] = "sndhdmi";
static const char From[] = "sndhdmi";

CHDMISoundBaseDevice::CHDMISoundBaseDevice (CInterruptSystem *pInterrupt,
					    unsigned nSampleRate,
					    unsigned nChunkSize)
:	CSoundBaseDevice (SoundFormatIEC958, 0, nSampleRate),
	m_pInterruptSystem (pInterrupt),
	m_nSampleRate (nSampleRate),
	m_nChunkSize (nChunkSize),
	m_ulAudioClockRate (0),
	m_ulPixelClockRate (0),
	m_bUsePolling (FALSE),
	m_bIRQConnected (FALSE),
	m_State (HDMISoundCreated),
	m_nDMAChannel (CMachineInfo::Get ()->AllocateDMAChannel (DMA_CHANNEL_LITE))
{
	assert (m_pInterruptSystem != 0);
	assert (m_nSampleRate > 0);
	assert (m_nChunkSize % IEC958_SUBFRAMES_PER_BLOCK == 0);

	if (m_nDMAChannel > DMA_CHANNEL_MAX)	// no DMA channel assigned
	{
		m_State = HDMISoundError;

		return;
	}

	// setup and concatenate DMA buffers and control blocks
	SetupDMAControlBlock (0);
	SetupDMAControlBlock (1);
	m_pControlBlock[0]->nNextControlBlockAddress = BUS_ADDRESS ((uintptr) m_pControlBlock[1]);
	m_pControlBlock[1]->nNextControlBlockAddress = BUS_ADDRESS ((uintptr) m_pControlBlock[0]);

	CDeviceNameService::Get ()->AddDevice (DeviceName, this, FALSE);
}

CHDMISoundBaseDevice::CHDMISoundBaseDevice (unsigned nSampleRate)
:	CSoundBaseDevice (SoundFormatIEC958, 0, nSampleRate),
	m_pInterruptSystem (0),
	m_nSampleRate (nSampleRate),
	m_nChunkSize (0),
	m_ulAudioClockRate (0),
	m_ulPixelClockRate (0),
	m_bUsePolling (TRUE),
	m_nSubFrame (0),
	m_bIRQConnected (FALSE),
	m_State (HDMISoundCreated),
	m_nDMAChannel (DMA_CHANNEL_MAX)		// no DMA channel assigned
{
	assert (m_nSampleRate > 0);

	CDeviceNameService::Get ()->AddDevice (DeviceName, this, FALSE);
}

CHDMISoundBaseDevice::~CHDMISoundBaseDevice (void)
{
	assert (   m_State == HDMISoundCreated
		|| m_State == HDMISoundIdle
		|| m_State == HDMISoundError);

	if (   m_nDMAChannel > DMA_CHANNEL_MAX
	    && !m_bUsePolling)
	{
		return;
	}

	CDeviceNameService::Get ()->RemoveDevice (DeviceName, FALSE);

	ResetHDMI ();

	if (m_bUsePolling)
	{
		return;
	}

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

boolean CHDMISoundBaseDevice::Start (void)
{
	if (m_State > HDMISoundIdle)
	{
		return FALSE;
	}

	if (m_State == HDMISoundCreated)
	{
		PeripheralEntry ();

		if (!(read32 (RegRamPacketConfig) & BitRamPacketConfigEnable))
		{
			CLogger::Get ()->Write (From, LogError,
						"Requires HDMI display with audio support");

			m_State = HDMISoundError;

			return FALSE;
		}

		PeripheralExit ();

#if RASPPI <= 3
		m_ulAudioClockRate = GetHSMClockRate ();
		if (m_ulAudioClockRate == 0)
		{
			CLogger::Get ()->Write (From, LogError, "Cannot get HDMI clock rate");

			m_State = HDMISoundError;

			return FALSE;
		}

		m_ulPixelClockRate = GetPixelClockRate ();
		if (m_ulPixelClockRate == 0)
		{
			CLogger::Get ()->Write (From, LogWarning, "Cannot get pixel clock rate");

			m_State = HDMISoundError;

			return FALSE;
		}
		assert (m_ulPixelClockRate <= 162000000UL);
#else
		// TODO? manage clocks for RASPPI >= 4
		m_ulAudioClockRate = 108000000UL;
		m_ulPixelClockRate = CMachineInfo::Get ()->GetClockRate (CLOCK_ID_PIXEL_BVB);
		assert (m_ulPixelClockRate <= 340000000UL);
#endif

#ifdef HDMI_DEBUG
		CLogger::Get ()->Write (From, LogDebug, "HDMI clock rate is %lu Hz",
					m_ulAudioClockRate);
		CLogger::Get ()->Write (From, LogDebug, "Pixel clock rate is %lu Hz",
					m_ulPixelClockRate);
#endif

		if (!m_bUsePolling)
		{
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
		}

		RunHDMI ();

		m_State = HDMISoundIdle;
	}

	assert (m_State == HDMISoundIdle);

	// fill buffer 0
	m_nNextBuffer = 0;

	if (   !m_bUsePolling
	    && !GetNextChunk ())
	{
		return FALSE;
	}

	m_State = HDMISoundRunning;

	if (!m_bUsePolling)
	{
		// connect IRQ
		assert (m_nDMAChannel <= DMA_CHANNEL_MAX);

		if (!m_bIRQConnected)
		{
			assert (m_pInterruptSystem != 0);
			m_pInterruptSystem->ConnectIRQ (ARM_IRQ_DMA0+m_nDMAChannel, InterruptStub, this);

			m_bIRQConnected = TRUE;
		}
	}

	// enable HDMI audio operation
	PeripheralEntry ();

#if RASPPI <= 3
	write32 (RegTxPhyControl0, read32 (RegTxPhyControl0) & ~BitTxPhyControl0RngPowerDown);
#else
	write32 (RegTxPhyPowerDownControl,
		 read32 (RegTxPhyPowerDownControl) & ~BitTxPhyPowerDownControlRngGenPowerDown);
#endif

	write32 (RegMaiControl,  IEC958_HW_CHANNELS << ShiftMaiControlChannelNumber
			       | BitMaiControlWholSample
			       | BitMaiControlChannelAlign
			       | BitMaiControlEnable);

	PeripheralExit ();

	if (m_bUsePolling)
	{
		return TRUE;
	}

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

		if (m_State == HDMISoundRunning)
		{
			PeripheralEntry ();
			write32 (ARM_DMACHAN_NEXTCONBK (m_nDMAChannel), 0);
			PeripheralExit ();

			StopHDMI ();

			m_State = HDMISoundTerminating;
		}

		m_SpinLock.Release ();
	}

	return TRUE;
}

void CHDMISoundBaseDevice::Cancel (void)
{
	if (m_bUsePolling)
	{
		if (m_State == HDMISoundRunning)
		{
			StopHDMI ();

			m_State = HDMISoundIdle;
		}

		return;
	}

	m_SpinLock.Acquire ();

	if (m_State == HDMISoundRunning)
	{
		m_State = HDMISoundCancelled;
	}

	m_SpinLock.Release ();
}

boolean CHDMISoundBaseDevice::IsActive (void) const
{
	return    m_State == HDMISoundRunning
	       || m_State == HDMISoundCancelled
	       || m_State == HDMISoundTerminating;
}

boolean CHDMISoundBaseDevice::IsWritable (void)
{
	assert (m_bUsePolling);

	PeripheralEntry ();

	boolean bResult = !(read32 (RegMaiControl) & BitMaiControlFull);

	PeripheralExit ();

	return bResult;
}

void CHDMISoundBaseDevice::WriteSample (s32 nSample)
{
	assert (m_bUsePolling);

	PeripheralEntry ();

	write32 (RegMaiData, ConvertIEC958Sample (nSample, m_nSubFrame / IEC958_HW_CHANNELS));

	PeripheralExit ();

	if (++m_nSubFrame == IEC958_SUBFRAMES_PER_BLOCK)
	{
		m_nSubFrame = 0;
	}
}

boolean CHDMISoundBaseDevice::GetNextChunk (void)
{
	assert (m_pDMABuffer[m_nNextBuffer] != 0);

	unsigned nChunkSize = GetChunk (m_pDMABuffer[m_nNextBuffer], m_nChunkSize);
	if (nChunkSize == 0)
	{
		return FALSE;
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

void CHDMISoundBaseDevice::RunHDMI (void)
{
	assert (m_nSampleRate > 0);
	assert (m_ulPixelClockRate > 0);

	unsigned long ulNumerator, ulDenominator;
	rational_best_approximation (m_ulAudioClockRate, m_nSampleRate,
				     MaskMaiSampleRateN >> ShiftMaiSampleRateN,
				     (MaskMaiSampleRateM >> ShiftMaiSampleRateM) + 1,
				     &ulNumerator, &ulDenominator);

	u32 nSampleRateMul128 = m_nSampleRate * 128;
	u32 nN = nSampleRateMul128 / 1000;
	u32 nCTS = (u32) (((u64) m_ulPixelClockRate * nN) / nSampleRateMul128);

	struct
	{
		uintptr	nReg;
		u32	nValue;
	}
	InitValues[] =
	{
		{RegMaiControl,   BitMaiControlReset | BitMaiControlFlush | BitMaiControlDelayed
				| BitMaiControlErrorEmpty | BitMaiControlErrorFull},
		{RegMaiSampleRate,   (u32) ulNumerator << ShiftMaiSampleRateN
				   | (u32) (ulDenominator - 1) << ShiftMaiSampleRateM},
		{RegMaiFormat,   SampleRateToHWFormat (m_nSampleRate) << ShiftMaiFormatSampleRate
			       | MaiFormatAudioFormatPCM << ShiftMaiFormatAudioFormat},
		{RegMaiThreshold,   MaiThresholdAnyDefault << ShiftMaiThresholdPanicHigh
				  | MaiThresholdAnyDefault << ShiftMaiThresholdPanicLow
				  | MaiThresholdAnyDefault << ShiftMaiThresholdDREQHigh
				  | MaiThresholdAnyDefault << ShiftMaiThresholdDREQLow},
		{RegMaiConfig,   BitMaiConfigBitReverse | BitMaiConfigFormatReverse
			       | 0b11 << ShiftMaiConfigChannelMask},
#if RASPPI <= 3
		{RegMaiChannelMap, 0b001000},
#else
		{RegMaiChannelMap, 0b00010000},
#endif
		{RegAudioPacketConfig,   BitAudioPacketConfigZeroDataOnSampleFlat
				       | BitAudioPacketConfigZeroDataOnInactiveChannels
				       |    IEC958_B_FRAME_PREAMBLE
				         << ShiftAudioPacketConfigBFrameIdentifier
				       | 0b11 << ShiftAudioPacketConfigCeaMask},
		{RegCrpConfig, BitCrpConfigExternalCtsEnable | nN << ShiftCrpConfigConfigN},
		{RegCts0, nCTS},
		{RegCts1, nCTS},
		{0}
	};

	PeripheralEntry ();

	for (unsigned i = 0; InitValues[i].nReg != 0; i++)
	{
		write32 (InitValues[i].nReg, InitValues[i].nValue);
	}

#ifdef HDMI_DEBUG
	for (unsigned i = 0; InitValues[i].nReg != 0; i++)
	{
		CLogger::Get ()->Write (From, LogDebug, "%08X: %08X",
					InitValues[i].nReg, read32 (InitValues[i].nReg));
	}
#endif

	if (!SetAudioInfoFrame ())
	{
		CLogger::Get ()->Write (From, LogError, "Cannot set audio infoframe");
	}

	PeripheralExit ();
}

void CHDMISoundBaseDevice::StopHDMI (void)
{
	PeripheralEntry ();

	write32 (RegMaiControl, BitMaiControlDelayed | BitMaiControlErrorEmpty | BitMaiControlErrorFull);

#if RASPPI <= 3
	write32 (RegTxPhyControl0, read32 (RegTxPhyControl0) | BitTxPhyControl0RngPowerDown);
#else
	write32 (RegTxPhyPowerDownControl,
		 read32 (RegTxPhyPowerDownControl) | BitTxPhyPowerDownControlRngGenPowerDown);
#endif

	PeripheralExit ();
}

void CHDMISoundBaseDevice::InterruptHandler (void)
{
	assert (m_State != HDMISoundIdle);
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
		StopHDMI ();

		m_State = HDMISoundError;

		return;
	}

	m_SpinLock.Acquire ();

	switch (m_State)
	{
	case HDMISoundRunning:
		if (GetNextChunk ())
		{
			break;
		}
		// fall through

	case HDMISoundCancelled:
		PeripheralEntry ();
		write32 (ARM_DMACHAN_NEXTCONBK (m_nDMAChannel), 0);
		PeripheralExit ();

		m_State = HDMISoundTerminating;
		break;

	case HDMISoundTerminating:
		StopHDMI ();

		m_State = HDMISoundIdle;
		break;

	default:
		assert (0);
		break;
	}

	m_SpinLock.Release ();
}

void CHDMISoundBaseDevice::InterruptStub (void *pParam)
{
	CHDMISoundBaseDevice *pThis = (CHDMISoundBaseDevice *) pParam;
	assert (pThis != 0);

	pThis->InterruptHandler ();
}

void CHDMISoundBaseDevice::SetupDMAControlBlock (unsigned nID)
{
	assert (nID <= 1);

	m_pDMABuffer[nID] = new (HEAP_DMA30) u32[m_nChunkSize];
	assert (m_pDMABuffer[nID] != 0);

	m_pControlBlockBuffer[nID] = new (HEAP_DMA30) u8[sizeof (TDMAControlBlock) + 31];
	assert (m_pControlBlockBuffer[nID] != 0);
	m_pControlBlock[nID] = (TDMAControlBlock *) (((uintptr) m_pControlBlockBuffer[nID] + 31) & ~31);

	m_pControlBlock[nID]->nTransferInformation     =   (DREQSourceHDMI << TI_PERMAP_SHIFT)
						         | (DEFAULT_BURST_LENGTH << TI_BURST_LENGTH_SHIFT)
						         | TI_SRC_INC
						         | TI_SRC_DREQ
						         | TI_WAIT_RESP
						         | TI_INTEN;
	m_pControlBlock[nID]->nSourceAddress           = BUS_ADDRESS ((uintptr) m_pDMABuffer[nID]);
	m_pControlBlock[nID]->nDestinationAddress      = (RegMaiData & 0xFFFFFF) + GPU_IO_BASE;
	m_pControlBlock[nID]->n2DModeStride            = 0;
	m_pControlBlock[nID]->nReserved[0]	       = 0;
	m_pControlBlock[nID]->nReserved[1]	       = 0;
}

void CHDMISoundBaseDevice::ResetHDMI (void)
{
	PeripheralEntry ();

	write32 (RegRamPacketConfig,
		 read32 (RegRamPacketConfig) & ~BitRamPacketConfigAudioPacketIdentifier);
	if (!WaitForBit (RegRamPacketStatus, BitRamPacketStatusAudioPacketIdentifier, FALSE, 100))
	{
		CLogger::Get ()->Write (From, LogWarning, "Cannot stop audio infoframe");
	}

	write32 (RegMaiControl, BitMaiControlReset);
	write32 (RegMaiControl, BitMaiControlErrorFull);
	write32 (RegMaiControl, BitMaiControlFlush);

	PeripheralExit ();
}

boolean CHDMISoundBaseDevice::SetAudioInfoFrame (void)
{
	assert (read32 (RegRamPacketConfig) & BitRamPacketConfigEnable);

	write32 (RegRamPacketConfig,
		 read32 (RegRamPacketConfig) & ~BitRamPacketConfigAudioPacketIdentifier);
	if (!WaitForBit (RegRamPacketStatus, BitRamPacketStatusAudioPacketIdentifier, FALSE, 100))
	{
		return FALSE;
	}

	write32 (RegRamPacketAudio0, 0x0A0184);		// type, version, length
	write32 (RegRamPacketAudio1, 0x0170);		// checksum, channels-1

	for (uintptr nReg = RegRamPacketAudio2; nReg <= RegRamPacketAudio8; nReg += 4)
	{
		write32 (nReg, 0);
	}

#ifdef HDMI_DEBUG
	for (uintptr nReg = RegRamPacketAudio0; nReg <= RegRamPacketAudio8; nReg += 4)
	{
		CLogger::Get ()->Write (From, LogDebug, "%08X: %08X", nReg, read32 (nReg));
	}
#endif

	write32 (RegRamPacketConfig,
		 read32 (RegRamPacketConfig) | BitRamPacketConfigAudioPacketIdentifier);
	return WaitForBit (RegRamPacketStatus, BitRamPacketStatusAudioPacketIdentifier, TRUE, 100);
}

u32 CHDMISoundBaseDevice::SampleRateToHWFormat (unsigned nSampleRate)
{
	static const unsigned HWSampleRate[] =
	{
		8000,  11025, 12000, 16000, 22050,  24000,  32000,  44100,
		48000, 64000, 88200, 96000, 128000, 176400, 192000, 0
	};

	for (u32 i = 0; HWSampleRate[i] != 0; i++)
	{
		if (HWSampleRate[i] == nSampleRate)
		{
			return MaiFormatSampleRate8000 + i;
		}
	}

	return MaiFormatSampleRateNotIndicated;
}

boolean CHDMISoundBaseDevice::WaitForBit (uintptr nIOAddress, u32 nMask, boolean bWaitUntilSet,
					  unsigned nMsTimeout)
{
	assert (nMask != 0);
	assert (nMsTimeout > 0);

	while ((read32 (nIOAddress) & nMask) ? !bWaitUntilSet : bWaitUntilSet)
	{
		CTimer::Get ()->MsDelay (1);

		if (--nMsTimeout == 0)
		{
			return FALSE;
		}
	}

	return TRUE;
}

#if RASPPI <= 3

// Portions from linux/driver/clk/bcm/clk-bcm2835.c:
// Copyright (C) 2010,2015 Broadcom
// Copyright (C) 2012 Stephen Warren
// SPDX-License-Identifier: GPL-2.0+

#define CM_HSMCTL		(ARM_CM_BASE + 0x088)
#define CM_HSMDIV		(ARM_CM_BASE + 0x08c)
#define CM_DIV_FRAC_BITS	12

unsigned long CHDMISoundBaseDevice::GetHSMClockRate (void)
{
	unsigned long ulParentRate =
		CMachineInfo::Get ()->GetGPIOClockSourceRate (GPIOClockSourcePLLD);
	if (ulParentRate == 0)
	{
		return 0;
	}

	u32 div = read32 (CM_HSMDIV);

	/*
	 * The divisor is a 12.12 fixed point field, but only some of
	 * the bits are populated in any given clock.
	 */
	static const u32 int_bits = 4;
	static const u32 frac_bits = 8;
	div >>= CM_DIV_FRAC_BITS - frac_bits;
	div &= (1 << (int_bits + frac_bits)) - 1;

	if (div == 0)
	{
		return 0;
	}

	u64 rate = (u64) ulParentRate << frac_bits;
	rate /= div;

	return rate;
}

#define A2W_PLLH_CTRLR			(ARM_CM_BASE + 0x1960)
#define A2W_PLLH_FRACR			(ARM_CM_BASE + 0x1A60)
#define A2W_PLLH_ANA0			(ARM_CM_BASE + 0x1070)
#define PLLH_FB_PREDIV_MASK		BIT(11)

#define A2W_PLL_FRAC_BITS		20
#define A2W_PLL_FRAC_MASK		((1 << A2W_PLL_FRAC_BITS) - 1)
#define A2W_PLL_CTRL_PDIV_MASK		0x7000
#define A2W_PLL_CTRL_PDIV_SHIFT		12
#define A2W_PLL_CTRL_NDIV_MASK		0x3FF
#define A2W_PLL_CTRL_NDIV_SHIFT		0

#define PLLH_PIX_FIXED_DIVIDER		10UL

unsigned long CHDMISoundBaseDevice::GetPixelClockRate (void)
{
	unsigned long ulParentRate =
		CMachineInfo::Get ()->GetGPIOClockSourceRate (GPIOClockSourceOscillator);
	if (ulParentRate == 0)
	{
		return 0;
	}

	u32 a2wctrl = read32 (A2W_PLLH_CTRLR);
	u32 fdiv = read32 (A2W_PLLH_FRACR) & A2W_PLL_FRAC_MASK;
	u32 ndiv = (a2wctrl & A2W_PLL_CTRL_NDIV_MASK) >> A2W_PLL_CTRL_NDIV_SHIFT;
	u32 pdiv = (a2wctrl & A2W_PLL_CTRL_PDIV_MASK) >> A2W_PLL_CTRL_PDIV_SHIFT;

	if (read32 (A2W_PLLH_ANA0 + 4) & PLLH_FB_PREDIV_MASK)
	{
		ndiv *= 2;
		fdiv *= 2;
	}

	if (pdiv == 0)
	{
		return 0;
	}

	u64 rate = (u64) ulParentRate * ((ndiv << A2W_PLL_FRAC_BITS) + fdiv);
	rate /= pdiv;
	rate >>= A2W_PLL_FRAC_BITS;

	rate /= PLLH_PIX_FIXED_DIVIDER;

	return rate;
}

#endif // #if RASPPI <= 3

/*
 * From linux/lib/math/rational.c:
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * rational fractions
 *
 * Copyright (C) 2009 emlix GmbH, Oskar Schirmer <oskar@scara.com>
 * Copyright (C) 2019 Trent Piepho <tpiepho@gmail.com>
 *
 * helper functions when coping with rational numbers
 */

/*
 * calculate best rational approximation for a given fraction
 * taking into account restricted register size, e.g. to find
 * appropriate values for a pll with 5 bit denominator and
 * 8 bit numerator register fields, trying to set up with a
 * frequency ratio of 3.1415, one would say:
 *
 * rational_best_approximation(31415, 10000,
 *		(1 << 8) - 1, (1 << 5) - 1, &n, &d);
 *
 * you may look at given_numerator as a fixed point number,
 * with the fractional part size described in given_denominator.
 *
 * for theoretical background, see:
 * https://en.wikipedia.org/wiki/Continued_fraction
 */

#define min(a, b)	(((a) < (b)) ? (a) : (b))

void CHDMISoundBaseDevice::rational_best_approximation(
	unsigned long given_numerator, unsigned long given_denominator,
	unsigned long max_numerator, unsigned long max_denominator,
	unsigned long *best_numerator, unsigned long *best_denominator)
{
	/* n/d is the starting rational, which is continually
	 * decreased each iteration using the Euclidean algorithm.
	 *
	 * dp is the value of d from the prior iteration.
	 *
	 * n2/d2, n1/d1, and n0/d0 are our successively more accurate
	 * approximations of the rational.  They are, respectively,
	 * the current, previous, and two prior iterations of it.
	 *
	 * a is current term of the continued fraction.
	 */
	unsigned long n, d, n0, d0, n1, d1, n2, d2;
	n = given_numerator;
	d = given_denominator;
	n0 = d1 = 0;
	n1 = d0 = 1;

	for (;;) {
		unsigned long dp, a;

		if (d == 0)
			break;
		/* Find next term in continued fraction, 'a', via
		 * Euclidean algorithm.
		 */
		dp = d;
		a = n / d;
		d = n % d;
		n = dp;

		/* Calculate the current rational approximation (aka
		 * convergent), n2/d2, using the term just found and
		 * the two prior approximations.
		 */
		n2 = n0 + a * n1;
		d2 = d0 + a * d1;

		/* If the current convergent exceeds the maxes, then
		 * return either the previous convergent or the
		 * largest semi-convergent, the final term of which is
		 * found below as 't'.
		 */
		if ((n2 > max_numerator) || (d2 > max_denominator)) {
			unsigned long t = min((max_numerator - n0) / n1,
					      (max_denominator - d0) / d1);

			/* This tests if the semi-convergent is closer
			 * than the previous convergent.
			 */
			if (2u * t > a || (2u * t == a && d0 * dp > d1 * d)) {
				n1 = n0 + t * n1;
				d1 = d0 + t * d1;
			}
			break;
		}
		n0 = n1;
		n1 = n2;
		d0 = d1;
		d1 = d2;
	}
	*best_numerator = n1;
	*best_denominator = d1;
}
