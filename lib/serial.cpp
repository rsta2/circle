//
// serial.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
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
#include <circle/serial.h>
#include <circle/devicenameservice.h>
#include <circle/bcm2835.h>
#include <circle/memio.h>
#include <circle/machineinfo.h>
#include <circle/synchronize.h>
#include <assert.h>

#ifndef USE_RPI_STUB_AT

#define GPIOS			2
#define GPIO_TXD		0
#define GPIO_RXD		1

#define VALUES			2
#define VALUE_PIN		0
#define VALUE_ALT		1

// Registers
#define ARM_UART_DR		(m_nBaseAddress + 0x00)
#define ARM_UART_FR     	(m_nBaseAddress + 0x18)
#define ARM_UART_IBRD   	(m_nBaseAddress + 0x24)
#define ARM_UART_FBRD   	(m_nBaseAddress + 0x28)
#define ARM_UART_LCRH   	(m_nBaseAddress + 0x2C)
#define ARM_UART_CR     	(m_nBaseAddress + 0x30)
#define ARM_UART_IFLS   	(m_nBaseAddress + 0x34)
#define ARM_UART_IMSC   	(m_nBaseAddress + 0x38)
#define ARM_UART_RIS    	(m_nBaseAddress + 0x3C)
#define ARM_UART_MIS    	(m_nBaseAddress + 0x40)
#define ARM_UART_ICR    	(m_nBaseAddress + 0x44)

// Definitions from Raspberry PI Remote Serial Protocol.
//     Copyright 2012 Jamie Iles, jamie@jamieiles.com.
//     Licensed under GPLv2
#define DR_OE_MASK		(1 << 11)
#define DR_BE_MASK		(1 << 10)
#define DR_PE_MASK		(1 << 9)
#define DR_FE_MASK		(1 << 8)

#define FR_TXFE_MASK		(1 << 7)
#define FR_RXFF_MASK		(1 << 6)
#define FR_TXFF_MASK		(1 << 5)
#define FR_RXFE_MASK		(1 << 4)
#define FR_BUSY_MASK		(1 << 3)

#define LCRH_SPS_MASK		(1 << 7)
#define LCRH_WLEN8_MASK		(3 << 5)
#define LCRH_WLEN7_MASK		(2 << 5)
#define LCRH_WLEN6_MASK		(1 << 5)
#define LCRH_WLEN5_MASK		(0 << 5)
#define LCRH_FEN_MASK		(1 << 4)
#define LCRH_STP2_MASK		(1 << 3)
#define LCRH_EPS_MASK		(1 << 2)
#define LCRH_PEN_MASK		(1 << 1)
#define LCRH_BRK_MASK		(1 << 0)

#define CR_CTSEN_MASK		(1 << 15)
#define CR_RTSEN_MASK		(1 << 14)
#define CR_OUT2_MASK		(1 << 13)
#define CR_OUT1_MASK		(1 << 12)
#define CR_RTS_MASK		(1 << 11)
#define CR_DTR_MASK		(1 << 10)
#define CR_RXE_MASK		(1 << 9)
#define CR_TXE_MASK		(1 << 8)
#define CR_LBE_MASK		(1 << 7)
#define CR_UART_EN_MASK		(1 << 0)

#define IFLS_RXIFSEL_SHIFT	3
#define IFLS_RXIFSEL_MASK	(7 << IFLS_RXIFSEL_SHIFT)
#define IFLS_TXIFSEL_SHIFT	0
#define IFLS_TXIFSEL_MASK	(7 << IFLS_TXIFSEL_SHIFT)
	#define IFLS_IFSEL_1_8		0
	#define IFLS_IFSEL_1_4		1
	#define IFLS_IFSEL_1_2		2
	#define IFLS_IFSEL_3_4		3
	#define IFLS_IFSEL_7_8		4

#define INT_OE			(1 << 10)
#define INT_BE			(1 << 9)
#define INT_PE			(1 << 8)
#define INT_FE			(1 << 7)
#define INT_RT			(1 << 6)
#define INT_TX			(1 << 5)
#define INT_RX			(1 << 4)
#define INT_DSRM		(1 << 3)
#define INT_DCDM		(1 << 2)
#define INT_CTSM		(1 << 1)

static uintptr s_BaseAddress[SERIAL_DEVICES] =
{
	ARM_IO_BASE + 0x201000,
#if RASPPI >= 4
	0,
	ARM_IO_BASE + 0x201400,
	ARM_IO_BASE + 0x201600,
	ARM_IO_BASE + 0x201800,
	ARM_IO_BASE + 0x201A00
#endif
};

#define NONE	{10000, 10000}

static unsigned s_GPIOConfig[SERIAL_DEVICES][GPIOS][VALUES] =
{
	// TXD      RXD
#if SERIAL_GPIO_SELECT == 14
	{{14,  0}, {15,  0}},
#elif SERIAL_GPIO_SELECT == 32
	{{32,  3}, {33,  3}},
#elif SERIAL_GPIO_SELECT == 36
	{{36,  2}, {37,  2}},
#else
	#error SERIAL_GPIO_SELECT must be 14, 32 or 36!
#endif
#if RASPPI >= 4
	{  NONE,     NONE  }, // unused
	{{ 0,  4}, { 1,  4}},
	{{ 4,  4}, { 5,  4}},
	{{ 8,  4}, { 9,  4}},
	{{12,  4}, {13,  4}}
#endif
};

#define ALT_FUNC(device, gpio)	((TGPIOMode) (  s_GPIOConfig[device][gpio][VALUE_ALT] \
					      + GPIOModeAlternateFunction0))

unsigned CSerialDevice::s_nInterruptUseCount = 0;
CInterruptSystem *CSerialDevice::s_pInterruptSystem = 0;
boolean CSerialDevice::s_bUseFIQ = FALSE;
volatile u32 CSerialDevice::s_nInterruptDeviceMask = 0;
CSerialDevice *CSerialDevice::s_pThis[SERIAL_DEVICES] = {0};

CSerialDevice::CSerialDevice (CInterruptSystem *pInterruptSystem, boolean bUseFIQ, unsigned nDevice)
:	m_pInterruptSystem (pInterruptSystem),
	m_bUseFIQ (bUseFIQ),
	m_nDevice (nDevice),
	m_nBaseAddress (0),
	m_bValid (FALSE),
	m_nRxInPtr (0),
	m_nRxOutPtr (0),
	m_nRxStatus (0),
	m_nTxInPtr (0),
	m_nTxOutPtr (0),
	m_nOptions (SERIAL_OPTION_ONLCR),
	m_pMagic (0),
	m_SpinLock (bUseFIQ ? FIQ_LEVEL : IRQ_LEVEL)
#ifdef REALTIME
	, m_LineSpinLock (TASK_LEVEL)
#endif
{
	if (   m_nDevice >= SERIAL_DEVICES
	    || s_GPIOConfig[nDevice][0][VALUE_PIN] >= GPIO_PINS)
	{
		return;
	}

	assert (s_pThis[m_nDevice] == 0);
	s_pThis[m_nDevice] = this;

	m_nBaseAddress = s_BaseAddress[nDevice];
	assert (m_nBaseAddress != 0);

#if SERIAL_GPIO_SELECT == 14
	if (nDevice == 0)
	{
		// to be sure there is no collision with the Bluetooth controller
		m_GPIO32.AssignPin (32);
		m_GPIO32.SetMode (GPIOModeInput);

		m_GPIO33.AssignPin (33);
		m_GPIO33.SetMode (GPIOModeInput);
	}
#endif

	m_TxDPin.AssignPin (s_GPIOConfig[nDevice][GPIO_TXD][VALUE_PIN]);
	m_TxDPin.SetMode (ALT_FUNC (nDevice, GPIO_TXD));

	m_RxDPin.AssignPin (s_GPIOConfig[nDevice][GPIO_RXD][VALUE_PIN]);
	m_RxDPin.SetMode (ALT_FUNC (nDevice, GPIO_RXD));
	m_RxDPin.SetPullMode (GPIOPullModeUp);

	m_bValid = TRUE;
}

CSerialDevice::~CSerialDevice (void)
{
	if (!m_bValid)
	{
		return;
	}

	CDeviceNameService::Get ()->RemoveDevice ("ttyS", m_nDevice+1, FALSE);

	// remove device from interrupt handling
	s_nInterruptDeviceMask &= ~(1 << m_nDevice);
	DataSyncBarrier ();

	PeripheralEntry ();
	write32 (ARM_UART_IMSC, 0);
	write32 (ARM_UART_CR, 0);
	PeripheralExit ();

	// disconnect interrupt, if this is the last device, which uses interrupts
	if (   m_pInterruptSystem != 0
	    && --s_nInterruptUseCount == 0)
	{
		assert (s_pInterruptSystem != 0);
		if (!s_bUseFIQ)
		{
			s_pInterruptSystem->DisconnectIRQ (ARM_IRQ_UART);
		}
		else
		{
			s_pInterruptSystem->DisconnectFIQ ();
		}

		s_pInterruptSystem = 0;
		s_bUseFIQ = FALSE;
	}

	m_TxDPin.SetMode (GPIOModeInput);
	m_RxDPin.SetMode (GPIOModeInput);

	s_pThis[m_nDevice] = 0;
	m_bValid = FALSE;
}

boolean CSerialDevice::Initialize (unsigned nBaudrate,
				   unsigned nDataBits, unsigned nStopBits, TParity Parity)
{
	if (!m_bValid)
	{
		return FALSE;
	}

	unsigned nClockRate = CMachineInfo::Get ()->GetClockRate (CLOCK_ID_UART);
	assert (nClockRate > 0);

	assert (300 <= nBaudrate && nBaudrate <= 4000000);
	unsigned nBaud16 = nBaudrate * 16;
	unsigned nIntDiv = nClockRate / nBaud16;
	assert (1 <= nIntDiv && nIntDiv <= 0xFFFF);
	unsigned nFractDiv2 = (nClockRate % nBaud16) * 8 / nBaudrate;
	unsigned nFractDiv = nFractDiv2 / 2 + nFractDiv2 % 2;
	assert (nFractDiv <= 0x3F);

	if (m_pInterruptSystem != 0)		// do we want to use interrupts?
	{
		if (s_nInterruptUseCount > 0)
		{
			// if there is already an interrupt enabled device,
			// interrupt configuration must be the same
			if (   m_pInterruptSystem != s_pInterruptSystem
			    || m_bUseFIQ != s_bUseFIQ)
			{
				s_pThis[m_nDevice] = 0;
				m_bValid = FALSE;

				return FALSE;
			}
		}
		else
		{
			// if we are the first interrupt enabled device,
			// connect interrupt with our configuration
			s_pInterruptSystem = m_pInterruptSystem;
			s_bUseFIQ = m_bUseFIQ;

			if (!s_bUseFIQ)
			{
				s_pInterruptSystem->ConnectIRQ (ARM_IRQ_UART, InterruptStub, 0);
			}
			else
			{
				s_pInterruptSystem->ConnectFIQ (ARM_FIQ_UART, InterruptStub, 0);
			}
		}

		assert (s_nInterruptUseCount < SERIAL_DEVICES);
		s_nInterruptUseCount++;
	}

	PeripheralEntry ();

	write32 (ARM_UART_IMSC, 0);
	write32 (ARM_UART_ICR,  0x7FF);
	write32 (ARM_UART_IBRD, nIntDiv);
	write32 (ARM_UART_FBRD, nFractDiv);

	// line parameters
	u32 nLCRH = LCRH_FEN_MASK;
	switch (nDataBits)
	{
	case 5:		nLCRH |= LCRH_WLEN5_MASK;	break;
	case 6:		nLCRH |= LCRH_WLEN6_MASK;	break;
	case 7:		nLCRH |= LCRH_WLEN7_MASK;	break;
	case 8:		nLCRH |= LCRH_WLEN8_MASK;	break;

	default:
		assert (0);
		break;
	}

	assert (1 <= nStopBits && nStopBits <= 2);
	if (nStopBits == 2)
	{
		nLCRH |= LCRH_STP2_MASK;
	}

	switch (Parity)
	{
	case ParityNone:
		break;

	case ParityOdd:
		nLCRH |= LCRH_PEN_MASK;
		break;

	case ParityEven:
		nLCRH |= LCRH_PEN_MASK | LCRH_EPS_MASK;
		break;

	default:
		assert (0);
		break;
	}

	if (m_pInterruptSystem != 0)
	{
		write32 (ARM_UART_IFLS,   IFLS_IFSEL_1_4 << IFLS_TXIFSEL_SHIFT
					| IFLS_IFSEL_1_4 << IFLS_RXIFSEL_SHIFT);
		write32 (ARM_UART_LCRH, nLCRH);
		write32 (ARM_UART_IMSC, INT_RX | INT_RT | INT_OE);

		// add device to interrupt handling
		s_nInterruptDeviceMask |= 1 << m_nDevice;
		DataSyncBarrier ();
	}
	else
	{
		write32 (ARM_UART_LCRH, nLCRH);
	}

	write32 (ARM_UART_CR, CR_UART_EN_MASK | CR_TXE_MASK | CR_RXE_MASK);

	PeripheralExit ();

	CDeviceNameService::Get ()->AddDevice ("ttyS", m_nDevice+1, this, FALSE);

	return TRUE;
}

int CSerialDevice::Write (const void *pBuffer, size_t nCount)
{
	assert (m_bValid);

#ifdef REALTIME
	// cannot write from IRQ_LEVEL to prevent deadlock, just ignore it
	if (CurrentExecutionLevel () > TASK_LEVEL)
	{
		return nCount;
	}
#endif

	m_LineSpinLock.Acquire ();

	u8 *pChar = (u8 *) pBuffer;
	assert (pChar != 0);

	int nResult = 0;

	while (nCount--)
	{
		if (!Write (*pChar))
		{
			break;
		}

		if (*pChar++ == '\n')
		{
			if (m_nOptions & SERIAL_OPTION_ONLCR)
			{
				if (!Write ('\r'))
				{
					break;
				}
			}
		}

		nResult++;
	}

	m_LineSpinLock.Release ();

	if (m_pInterruptSystem != 0)
	{
		m_SpinLock.Acquire ();

		if (m_nTxInPtr != m_nTxOutPtr)
		{
			PeripheralEntry ();

			while (m_nTxInPtr != m_nTxOutPtr)
			{
				if (!(read32 (ARM_UART_FR) & FR_TXFF_MASK))
				{
					write32 (ARM_UART_DR, m_TxBuffer[m_nTxOutPtr++]);
					m_nTxOutPtr &= SERIAL_BUF_MASK;
				}
				else
				{
					write32 (ARM_UART_IMSC, read32 (ARM_UART_IMSC) | INT_TX);

					break;
				}
			}

			PeripheralExit ();
		}

		m_SpinLock.Release ();
	}

	return nResult;
}

int CSerialDevice::Read (void *pBuffer, size_t nCount)
{
	assert (m_bValid);

	u8 *pChar = (u8 *) pBuffer;
	assert (pChar != 0);

	int nResult = 0;

	if (m_pInterruptSystem != 0)
	{
		m_SpinLock.Acquire ();

		if (m_nRxStatus < 0)
		{
			nResult = m_nRxStatus;
			m_nRxStatus = 0;
		}
		else
		{
			while (nCount > 0)
			{
				if (m_nRxInPtr == m_nRxOutPtr)
				{
					break;
				}

				*pChar++ = m_RxBuffer[m_nRxOutPtr++];
				m_nRxOutPtr &= SERIAL_BUF_MASK;

				nCount--;
				nResult++;
			}
		}

		m_SpinLock.Release ();
	}
	else
	{
		PeripheralEntry ();

		while (nCount > 0)
		{
			if (read32 (ARM_UART_FR) & FR_RXFE_MASK)
			{
				break;
			}

			u32 nDR = read32 (ARM_UART_DR);
			if (nDR & DR_BE_MASK)
			{
				nResult = -SERIAL_ERROR_BREAK;

				break;
			}
			else if (nDR & DR_OE_MASK)
			{
				nResult = -SERIAL_ERROR_OVERRUN;

				break;
			}
			else if (nDR & DR_FE_MASK)
			{
				nResult = -SERIAL_ERROR_FRAMING;

				break;
			}
			else if (nDR & DR_PE_MASK)
			{
				nResult = -SERIAL_ERROR_PARITY;

				break;
			}

			*pChar++ = nDR & 0xFF;

			nCount--;
			nResult++;
		}

		PeripheralExit ();
	}

	return nResult;
}

unsigned CSerialDevice::GetOptions (void) const
{
	return m_nOptions;
}

void CSerialDevice::SetOptions (unsigned nOptions)
{
	m_nOptions = nOptions;
}

void CSerialDevice::RegisterMagicReceivedHandler (const char *pMagic, TMagicReceivedHandler *pHandler)
{
	assert (m_pInterruptSystem != 0);
	assert (m_pMagic == 0);

	assert (pMagic != 0);
	assert (*pMagic != '\0');
	assert (pHandler != 0);

	m_pMagicReceivedHandler = pHandler;

	m_pMagicPtr = pMagic;
	m_pMagic = pMagic;		// enables the scanner
}

unsigned CSerialDevice::AvailableForWrite (void)
{
	assert (m_bValid);
	assert (m_pInterruptSystem != 0);

	m_SpinLock.Acquire ();

	unsigned nResult;
	if (m_nTxOutPtr <= m_nTxInPtr)
	{
		nResult = SERIAL_BUF_SIZE+m_nTxOutPtr-m_nTxInPtr-1;
	}
	else
	{
		nResult = m_nTxOutPtr-m_nTxInPtr-1;
	}

	m_SpinLock.Release ();

	return nResult;
}

unsigned CSerialDevice::AvailableForRead (void)
{
	assert (m_bValid);
	assert (m_pInterruptSystem != 0);

	m_SpinLock.Acquire ();

	unsigned nResult;
	if (m_nRxInPtr < m_nRxOutPtr)
	{
		nResult = SERIAL_BUF_SIZE+m_nRxInPtr-m_nRxOutPtr;
	}
	else
	{
		nResult = m_nRxInPtr-m_nRxOutPtr;
	}

	m_SpinLock.Release ();

	return nResult;
}

int CSerialDevice::Peek (void)
{
	assert (m_bValid);
	assert (m_pInterruptSystem != 0);

	m_SpinLock.Acquire ();

	int nResult = -1;
	if (m_nRxInPtr != m_nRxOutPtr)
	{
		nResult = m_RxBuffer[m_nRxOutPtr];
	}

	m_SpinLock.Release ();

	return nResult;
}

void CSerialDevice::Flush (void)
{
	PeripheralEntry ();

	while (read32 (ARM_UART_FR) & FR_BUSY_MASK)
	{
		// just wait
	}

	PeripheralExit ();
}

boolean CSerialDevice::Write (u8 uchChar)
{
	boolean bOK = TRUE;

	if (m_pInterruptSystem != 0)
	{
		m_SpinLock.Acquire ();

		if (((m_nTxInPtr+1) & SERIAL_BUF_MASK) != m_nTxOutPtr)
		{
			m_TxBuffer[m_nTxInPtr++] = uchChar;
			m_nTxInPtr &= SERIAL_BUF_MASK;
		}
		else
		{
			bOK = FALSE;
		}

		m_SpinLock.Release ();
	}
	else
	{
		PeripheralEntry ();

		while (read32 (ARM_UART_FR) & FR_TXFF_MASK)
		{
			// do nothing
		}

		write32 (ARM_UART_DR, uchChar);

		PeripheralExit ();
	}

	return bOK;
}

void CSerialDevice::InterruptHandler (void)
{
	boolean bMagicReceived = FALSE;

	m_SpinLock.Acquire ();

	PeripheralEntry ();

	// acknowledge pending interrupts
	write32 (ARM_UART_ICR, read32 (ARM_UART_MIS));

	while (!(read32 (ARM_UART_FR) & FR_RXFE_MASK))
	{
		u32 nDR = read32 (ARM_UART_DR);
		if (nDR & DR_BE_MASK)
		{
			if (m_nRxStatus == 0)
			{
				m_nRxStatus = -SERIAL_ERROR_BREAK;
			}
		}
		else if (nDR & DR_OE_MASK)
		{
			if (m_nRxStatus == 0)
			{
				m_nRxStatus = -SERIAL_ERROR_OVERRUN;
			}
		}
		else if (nDR & DR_FE_MASK)
		{
			if (m_nRxStatus == 0)
			{
				m_nRxStatus = -SERIAL_ERROR_FRAMING;
			}
		}
		else if (nDR & DR_PE_MASK)
		{
			if (m_nRxStatus == 0)
			{
				m_nRxStatus = -SERIAL_ERROR_PARITY;
			}
		}

		if (m_pMagic != 0)
		{
			if ((char) (nDR & 0xFF) == *m_pMagicPtr)
			{
				if (*++m_pMagicPtr == '\0')
				{
					bMagicReceived = TRUE;
				}
			}
			else
			{
				m_pMagicPtr = m_pMagic;
			}
		}

		if (((m_nRxInPtr+1) & SERIAL_BUF_MASK) != m_nRxOutPtr)
		{
			m_RxBuffer[m_nRxInPtr++] = nDR & 0xFF;
			m_nRxInPtr &= SERIAL_BUF_MASK;
		}
		else
		{
			if (m_nRxStatus == 0)
			{
				m_nRxStatus = -SERIAL_ERROR_OVERRUN;
			}
		}
	}

	while (!(read32 (ARM_UART_FR) & FR_TXFF_MASK))
	{
		if (m_nTxInPtr != m_nTxOutPtr)
		{
			write32 (ARM_UART_DR, m_TxBuffer[m_nTxOutPtr++]);
			m_nTxOutPtr &= SERIAL_BUF_MASK;
		}
		else
		{
			write32 (ARM_UART_IMSC, read32 (ARM_UART_IMSC) & ~INT_TX);

			break;
		}
	}

	PeripheralExit ();

	m_SpinLock.Release ();

	if (bMagicReceived)
	{
		(*m_pMagicReceivedHandler) ();
	}
}

void CSerialDevice::InterruptStub (void *pParam)
{
	DataMemBarrier ();
	u32 nDeviceMask = s_nInterruptDeviceMask;

#if RASPPI >= 4
	for (unsigned i = 0; nDeviceMask != 0 && i < SERIAL_DEVICES; nDeviceMask &= ~(1 << i), i++)
	{
		if (nDeviceMask & (1 << i))
		{
			assert (s_pThis[i] != 0);
			s_pThis[i]->InterruptHandler ();
		}
	}
#else
	// only device 0 is supported here
	if (nDeviceMask & 1)
	{
		assert (s_pThis[0] != 0);
		s_pThis[0]->InterruptHandler ();
	}
#endif
}

#else	// #ifndef USE_RPI_STUB_AT

boolean CSerialDevice::Initialize (unsigned nBaudrate)
{
	CDeviceNameService::Get ()->AddDevice ("ttyS1", this, FALSE);

	return TRUE;
}

int CSerialDevice::Write (const void *pBuffer, unsigned nCount)
{
	asm volatile
	(
		"push {r0-r2}\n"
		"mov r0, %0\n"
		"mov r1, %1\n"
		"bkpt #0x7FFB\n"	// send message to GDB client
		"pop {r0-r2}\n"

		: : "r" (pBuffer), "r" (nCount)
	);

	return nCount;
}

#endif
