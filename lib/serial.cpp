//
// serial.cpp
//
// Based on Raspberry PI Remote Serial Protocol.
//     Copyright 2012 Jamie Iles, jamie@jamieiles.com.
//     Licensed under GPLv2
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
#include <circle/serial.h>
#include <circle/devicenameservice.h>
#include <circle/bcm2835.h>
#include <circle/memio.h>
#include <circle/machineinfo.h>
#include <circle/synchronize.h>
#include <circle/sysconfig.h>
#include <assert.h>

#ifndef USE_RPI_STUB_AT

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

CSerialDevice::CSerialDevice (void)
:
#if RASPPI >= 2
	// to be sure there is no collision with the Bluetooth controller
	m_GPIO32 (32, GPIOModeInput),
	m_GPIO33 (33, GPIOModeInput),
#endif
	m_TxDPin (14, GPIOModeAlternateFunction0),
	m_RxDPin (15, GPIOModeAlternateFunction0)
#ifdef REALTIME
	, m_SpinLock (FALSE)
#endif
{
}

CSerialDevice::~CSerialDevice (void)
{
	PeripheralEntry ();
	write32 (ARM_UART0_IMSC, 0);
	write32 (ARM_UART0_CR, 0);
	PeripheralExit ();
}

boolean CSerialDevice::Initialize (unsigned nBaudrate)
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

	PeripheralEntry ();

	write32 (ARM_UART0_IMSC, 0);
	write32 (ARM_UART0_ICR,  0x7FF);
	write32 (ARM_UART0_IBRD, nIntDiv);
	write32 (ARM_UART0_FBRD, nFractDiv);
	write32 (ARM_UART0_LCRH, LCRH_WLEN8_MASK);	// 8N1
	write32 (ARM_UART0_IFLS, 0);
	write32 (ARM_UART0_CR,   CR_UART_EN_MASK | CR_TXE_MASK | CR_RXE_MASK);

	PeripheralExit ();

	CDeviceNameService::Get ()->AddDevice ("ttyS1", this, FALSE);

	return TRUE;
}

int CSerialDevice::Write (const void *pBuffer, unsigned nCount)
{
	m_SpinLock.Acquire ();

	PeripheralEntry ();

	u8 *pChar = (u8 *) pBuffer;
	assert (pChar != 0);
	
	int nResult = 0;

	while (nCount--)
	{
		Write (*pChar);

		if (*pChar++ == '\n')
		{
			Write ('\r');
		}

		nResult++;
	}

	PeripheralExit ();

	m_SpinLock.Release ();

	return nResult;
}

void CSerialDevice::Write (u8 nChar)
{
	while (read32 (ARM_UART0_FR) & FR_TXFF_MASK)
	{
		// do nothing
	}
		
	write32 (ARM_UART0_DR, nChar);
}

#else

CSerialDevice::CSerialDevice (void)
{
}

CSerialDevice::~CSerialDevice (void)
{
}

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
