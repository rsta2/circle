//
// btuarttransport.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2016  R. Stange <rsta2@o2online.de>
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
#include <circle/bt/btuarttransport.h>
#include <circle/devicenameservice.h>
#include <circle/bcm2835.h>
#include <circle/memio.h>
#include <circle/machineinfo.h>
#include <circle/synchronize.h>
#include <circle/logger.h>
#include <assert.h>

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

enum TBTUARTRxState
{
	RxStateStart,
	RxStateCommand,
	RxStateLength,
	RxStateParam,
	RxStateUnknown
};

#define HCI_PACKET_COMMAND	0x01
#define HCI_PACKET_ACL_DATA	0x02
#define HCI_PACKET_SYNCH_DATA	0x03
#define HCI_PACKET_EVENT	0x04

static const char FromBTUART[] = "btuart";

CBTUARTTransport::CBTUARTTransport (CInterruptSystem *pInterruptSystem)
:	// to be sure there is no collision with the UART GPIO interface
	m_GPIO14 (14, GPIOModeInput),
	m_GPIO15 (15, GPIOModeInput),
	m_TxDPin (32, GPIOModeAlternateFunction3),
	m_RxDPin (33, GPIOModeAlternateFunction3),
	m_pInterruptSystem (pInterruptSystem),
	m_bIRQConnected (FALSE),
	m_pEventHandler (0),
	m_nRxState (RxStateStart)
{
}

CBTUARTTransport::~CBTUARTTransport (void)
{
	PeripheralEntry ();
	write32 (ARM_UART0_IMSC, 0);
	write32 (ARM_UART0_CR, 0);
	PeripheralExit ();

	m_pEventHandler = 0;

	if (m_bIRQConnected)
	{
		assert (m_pInterruptSystem != 0);
		m_pInterruptSystem->DisconnectIRQ (ARM_IRQ_UART);
	}

	m_pInterruptSystem = 0;
}

boolean CBTUARTTransport::Initialize (unsigned nBaudrate)
{
	unsigned nClockRate = CMachineInfo::Get ()->GetClockRate (CLOCK_ID_UART);
	assert (nClockRate > 0);

	assert (300 <= nBaudrate && nBaudrate <= 3000000);
	unsigned nBaud16 = nBaudrate * 16;
	unsigned nIntDiv = nClockRate / nBaud16;
	assert (1 <= nIntDiv && nIntDiv <= 0xFFFF);
	unsigned nFractDiv2 = (nClockRate % nBaud16) * 8 / nBaudrate;
	unsigned nFractDiv = nFractDiv2 / 2 + nFractDiv2 % 2;
	assert (nFractDiv <= 0x3F);

	assert (m_pInterruptSystem != 0);
	m_pInterruptSystem->ConnectIRQ (ARM_IRQ_UART, IRQStub, this);
	m_bIRQConnected = TRUE;

	PeripheralEntry ();

	write32 (ARM_UART0_IMSC, 0);
	write32 (ARM_UART0_ICR,  0x7FF);
	write32 (ARM_UART0_IBRD, nIntDiv);
	write32 (ARM_UART0_FBRD, nFractDiv);
	write32 (ARM_UART0_IFLS, IFLS_IFSEL_1_4 << IFLS_RXIFSEL_SHIFT);
	write32 (ARM_UART0_LCRH, LCRH_WLEN8_MASK | LCRH_FEN_MASK);		// 8N1
	write32 (ARM_UART0_CR,   CR_UART_EN_MASK | CR_TXE_MASK | CR_RXE_MASK);
	write32 (ARM_UART0_IMSC, INT_RX | INT_RT | INT_OE);

	PeripheralExit ();

	CDeviceNameService::Get ()->AddDevice ("ttyBT1", this, FALSE);

	return TRUE;
}

boolean CBTUARTTransport::SendHCICommand (const void *pBuffer, unsigned nLength)
{
	PeripheralEntry ();

	Write (HCI_PACKET_COMMAND);

	u8 *pChar = (u8 *) pBuffer;
	assert (pChar != 0);
	
	while (nLength--)
	{
		Write (*pChar++);
	}

	PeripheralExit ();

	return TRUE;
}

void CBTUARTTransport::RegisterHCIEventHandler (TBTHCIEventHandler *pHandler)
{
	assert (m_pEventHandler == 0);
	m_pEventHandler = pHandler;
	assert (m_pEventHandler != 0);
}

void CBTUARTTransport::Write (u8 nChar)
{
	while (read32 (ARM_UART0_FR) & FR_TXFF_MASK)
	{
		// do nothing
	}
		
	write32 (ARM_UART0_DR, nChar);
}

void CBTUARTTransport::IRQHandler (void)
{
	PeripheralEntry ();

	u32 nMIS = read32 (ARM_UART0_MIS);
	if (nMIS & INT_OE)
	{
		CLogger::Get ()->Write (FromBTUART, LogPanic, "Overrun error");
	}

	write32 (ARM_UART0_ICR, nMIS);

	while (1)
	{
		if (read32 (ARM_UART0_FR) & FR_RXFE_MASK)
		{
			break;
		}

		u8 nData = read32 (ARM_UART0_DR) & 0xFF;

		switch (m_nRxState)
		{
		case RxStateStart:
			if (nData == HCI_PACKET_EVENT)
			{
				m_nRxInPtr = 0;
				m_nRxState = RxStateCommand;
			}
			break;

		case RxStateCommand:
			m_RxBuffer[m_nRxInPtr++] = nData;
			m_nRxState = RxStateLength;
			break;

		case RxStateLength:
			m_RxBuffer[m_nRxInPtr++] = nData;
			if (nData > 0)
			{
				m_nRxParamLength = nData;
				m_nRxState = RxStateParam;
			}
			else
			{
				if (m_pEventHandler != 0)
				{
					(*m_pEventHandler) (m_RxBuffer, m_nRxInPtr);
				}

				m_nRxState = RxStateStart;
			}
			break;

		case RxStateParam:
			assert (m_nRxInPtr < BT_MAX_HCI_EVENT_SIZE);
			m_RxBuffer[m_nRxInPtr++] = nData;
			if (--m_nRxParamLength == 0)
			{
				if (m_pEventHandler != 0)
				{
					(*m_pEventHandler) (m_RxBuffer, m_nRxInPtr);
				}

				m_nRxState = RxStateStart;
			}
			break;

		default:
			assert (0);
			break;
		}
	}

	PeripheralExit ();
}

void CBTUARTTransport::IRQStub (void *pParam)
{
	CBTUARTTransport *pThis = (CBTUARTTransport *) pParam;
	assert (pThis != 0);

	pThis->IRQHandler ();
}
