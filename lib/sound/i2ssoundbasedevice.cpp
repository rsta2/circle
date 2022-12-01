//
// i2ssoundbasedevice.cpp
//
// Supports:
//	BCM283x/BCM2711 I2S output and input
//	two 24-bit audio channels
//	sample rate up to 192 KHz
//	output tested with PCM5102A, PCM5122 and WM8960 DACs
//
// References:
//	https://www.raspberrypi.org/forums/viewtopic.php?f=44&t=8496
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
#include <circle/sound/i2ssoundbasedevice.h>
#include <circle/devicenameservice.h>
#include <circle/bcm2835.h>
#include <circle/bcm2835int.h>
#include <circle/memio.h>
#include <circle/timer.h>
#include <assert.h>

#define CHANS			2			// 2 I2S stereo channels
#define CHANLEN			32			// width of a channel slot in bits

//
// PCM / I2S registers
//
#define CS_A_STBY		(1 << 25)
#define CS_A_SYNC		(1 << 24)
#define CS_A_RXSEX		(1 << 23)
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
#define CS_A_RXON		(1 << 1)
#define CS_A_EN			(1 << 0)

#define MODE_A_CLKI		(1 << 22)
#define MODE_A_CLKM		(1 << 23)
#define MODE_A_FSI		(1 << 20)
#define MODE_A_FSM		(1 << 21)
#define MODE_A_FLEN__SHIFT	10
#define MODE_A_FSLEN__SHIFT	0

#define RXC_A_CH1WEX		(1 << 31)
#define RXC_A_CH1EN		(1 << 30)
#define RXC_A_CH1POS__SHIFT	20
#define RXC_A_CH1WID__SHIFT	16
#define RXC_A_CH2WEX		(1 << 15)
#define RXC_A_CH2EN		(1 << 14)
#define RXC_A_CH2POS__SHIFT	4
#define RXC_A_CH2WID__SHIFT	0

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
#define DREQ_A_RX__SHIFT	0
#define DREQ_A_RX__MASK		(0x7F << 0)

CI2SSoundBaseDevice::CI2SSoundBaseDevice (CInterruptSystem *pInterrupt,
					  unsigned	    nSampleRate,
					  unsigned	    nChunkSize,
					  bool		    bSlave,
					  CI2CMaster       *pI2CMaster,
					  u8                ucI2CAddress,
					  TDeviceMode       DeviceMode)
:	CSoundBaseDevice (SoundFormatSigned24_32, 0, nSampleRate),
	m_nChunkSize (nChunkSize),
	m_bSlave (bSlave),
	m_pI2CMaster (pI2CMaster),
	m_ucI2CAddress (ucI2CAddress),
	m_DeviceMode (DeviceMode),
	m_Clock (GPIOClockPCM, GPIOClockSourcePLLD),
	m_bError (FALSE),
	m_TXBuffers (TRUE, ARM_PCM_FIFO_A, DREQSourcePCMTX, nChunkSize, pInterrupt),
	m_RXBuffers (FALSE, ARM_PCM_FIFO_A, DREQSourcePCMRX, nChunkSize, pInterrupt),
	m_bControllerInited (FALSE),
	m_pController (nullptr)
{
	assert (m_nChunkSize >= 32);
	assert ((m_nChunkSize & 1) == 0);

	// start clock and I2S device
	if (!m_bSlave)
	{
		unsigned nClockFreq =
			CMachineInfo::Get ()->GetGPIOClockSourceRate (GPIOClockSourcePLLD);
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
	}

	RunI2S ();

	CDeviceNameService::Get ()->AddDevice ("sndi2s", this, FALSE);
}

CI2SSoundBaseDevice::~CI2SSoundBaseDevice (void)
{
	CDeviceNameService::Get ()->RemoveDevice ("sndi2s", FALSE);

	delete m_pController;
	m_pController = nullptr;

	// stop I2S device and clock
	StopI2S ();
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
	if (m_bError)
	{
		return FALSE;
	}

	if (!m_bControllerInited)
	{
		if (!ControllerFactory ())
		{
			m_bError = TRUE;

			return FALSE;
		}

		m_bControllerInited = TRUE;
	}

	// enable I2S DMA operation
	PeripheralEntry ();

	if (m_nChunkSize < 64)
	{
		assert (m_nChunkSize >= 32);
		if (m_DeviceMode != DeviceModeRXOnly)
		{
			write32 (ARM_PCM_DREQ_A,   (read32 (ARM_PCM_DREQ_A) & ~DREQ_A_TX__MASK)
						 | (0x18 << DREQ_A_TX__SHIFT));
		}

		if (m_DeviceMode != DeviceModeTXOnly)
		{
			write32 (ARM_PCM_DREQ_A,   (read32 (ARM_PCM_DREQ_A) & ~DREQ_A_RX__MASK)
						 | (0x18 << DREQ_A_RX__SHIFT));  // TODO
		}
	}

	write32 (ARM_PCM_CS_A, read32 (ARM_PCM_CS_A) | CS_A_DMAEN);

	PeripheralExit ();

	u32 nTXRXOn = 0;

	if (m_DeviceMode != DeviceModeRXOnly)
	{
		if (!m_TXBuffers.Start (TXCompletedHandler, this))
		{
			m_bError = TRUE;

			return FALSE;
		}

		nTXRXOn |= CS_A_TXON;
	}

	if (m_DeviceMode != DeviceModeTXOnly)
	{
		if (!m_RXBuffers.Start (RXCompletedHandler, this))
		{
			m_bError = TRUE;

			return FALSE;
		}

		nTXRXOn |= CS_A_RXON | CS_A_RXSEX;
	}

	// enable TX and/or RX
	PeripheralEntry ();

	write32 (ARM_PCM_CS_A, read32 (ARM_PCM_CS_A) | nTXRXOn);

	PeripheralExit ();

	return TRUE;
}

void CI2SSoundBaseDevice::Cancel (void)
{
	if (m_DeviceMode != DeviceModeRXOnly)
	{
		m_TXBuffers.Cancel ();
	}

	if (m_DeviceMode != DeviceModeTXOnly)
	{
		m_RXBuffers.Cancel ();
	}
}

boolean CI2SSoundBaseDevice::IsActive (void) const
{
	if (   m_DeviceMode != DeviceModeRXOnly
	    && m_TXBuffers.IsActive ())
	{
		return TRUE;
	}

	if (   m_DeviceMode != DeviceModeTXOnly
	    && m_RXBuffers.IsActive ())
	{
		return TRUE;
	}

	return FALSE;
}

CSoundController *CI2SSoundBaseDevice::GetController (void)
{
	return m_pController;
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

	write32 (ARM_PCM_RXC_A,   RXC_A_CH1WEX
				| RXC_A_CH1EN
				| (1 << RXC_A_CH1POS__SHIFT)
				| (0 << RXC_A_CH1WID__SHIFT)
				| RXC_A_CH2WEX
				| RXC_A_CH2EN
				| ((CHANLEN+1) << RXC_A_CH2POS__SHIFT)
				| (0 << RXC_A_CH2WID__SHIFT));

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

	// init GPIO pins
	unsigned nPinBase = 18;
	TGPIOMode GPIOMode = GPIOModeAlternateFunction0;

	// assign to P5 header on early models
	TMachineModel Model = CMachineInfo::Get ()->GetMachineModel ();
	if (   Model == MachineModelA
	    || Model == MachineModelBRelease2MB256
	    || Model == MachineModelBRelease2MB512)
	{
		nPinBase = 28;
		GPIOMode = GPIOModeAlternateFunction2;
	}

	m_PCMCLKPin.AssignPin (nPinBase);
	m_PCMCLKPin.SetMode (GPIOMode);
	m_PCMFSPin.AssignPin (nPinBase+1);
	m_PCMFSPin.SetMode (GPIOMode);

	if (m_DeviceMode != DeviceModeTXOnly)
	{
		m_PCMDINPin.AssignPin (nPinBase+2);
		m_PCMDINPin.SetMode (GPIOMode);
	}

	if (m_DeviceMode != DeviceModeRXOnly)
	{
		m_PCMDOUTPin.AssignPin (nPinBase+3);
		m_PCMDOUTPin.SetMode (GPIOMode);
	}

	// disable standby
	write32 (ARM_PCM_CS_A, read32 (ARM_PCM_CS_A) | CS_A_STBY);
	CTimer::Get ()->usDelay (50);

	// enable I2S
	write32 (ARM_PCM_CS_A, read32 (ARM_PCM_CS_A) | CS_A_EN);
	CTimer::Get ()->usDelay (10);

	PeripheralExit ();
}

void CI2SSoundBaseDevice::StopI2S (void)
{
	PeripheralEntry ();

	write32 (ARM_PCM_CS_A, 0);
	CTimer::Get ()->usDelay (50);

	PeripheralExit ();

	if (!m_bSlave)
	{
		m_Clock.Stop ();
	}

	// de-init GPIO pins
	m_PCMCLKPin.SetMode (GPIOModeInput);
	m_PCMFSPin.SetMode (GPIOModeInput);

	if (m_DeviceMode != DeviceModeTXOnly)
	{
		m_PCMDINPin.SetMode (GPIOModeInput);
	}

	if (m_DeviceMode != DeviceModeRXOnly)
	{
		m_PCMDOUTPin.SetMode (GPIOModeInput);
	}
}

unsigned CI2SSoundBaseDevice::TXCompletedHandler (boolean bStatus, u32 *pBuffer,
						  unsigned nChunkSize, void *pParam)
{
	CI2SSoundBaseDevice *pThis = (CI2SSoundBaseDevice *) pParam;
	assert (pThis != 0);

	if (!bStatus)
	{
		pThis->m_bError = TRUE;

		return 0;
	}

	return pThis->GetChunk (pBuffer, nChunkSize);
}

unsigned CI2SSoundBaseDevice::RXCompletedHandler (boolean bStatus, u32 *pBuffer,
						  unsigned nChunkSize, void *pParam)
{
	CI2SSoundBaseDevice *pThis = (CI2SSoundBaseDevice *) pParam;
	assert (pThis != 0);

	if (!bStatus)
	{
		pThis->m_bError = TRUE;

		return 0;
	}

	pThis->PutChunk (pBuffer, nChunkSize);

	return 0;
}

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
	m_pController = new CWM8960SoundController (m_pI2CMaster, m_ucI2CAddress);
	assert (m_pController);

	if (m_pController->Probe ())
	{
		return TRUE;
	}

	delete m_pController;
	m_pController = nullptr;

	return !m_ucI2CAddress;		// PCM5102A does not need a controller
}
