//
// softserial.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2022  R. Stange <rsta2@o2online.de>
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
#include "softserial.h"
#include <circle/synchronize.h>
#include <assert.h>

#define BPS		38400

#define BIT_TIME	(CLOCKHZ / BPS)
#define BUFMASK		(SOFTSERIAL_BUFSIZE-1)

CSoftSerialDevice::CSoftSerialDevice (unsigned nGPIOPinTxD, unsigned nGPIOPinRxD, CGPIOManager *pGPIOManager)
:	m_TxDPin (nGPIOPinTxD, GPIOModeOutput),
	m_RxDPin (nGPIOPinRxD, GPIOModeInput, pGPIOManager),
	m_bInterruptOn (FALSE),
	m_hTimer (0),
	m_bCurrentLevel (TRUE),
	m_bStartBitExpected (TRUE),
	m_nBitCount (9),
	m_nShift (0),
	m_nInPtr (0),
	m_nOutPtr (0)
{
	m_TxDPin.Write (HIGH);
}

CSoftSerialDevice::~CSoftSerialDevice (void)
{
	if (m_hTimer != 0)
	{
		CTimer::Get ()->CancelKernelTimer (m_hTimer);
	}

	if (m_bInterruptOn)
	{
		m_RxDPin.DisableInterrupt ();
		m_RxDPin.DisconnectInterrupt ();
	}
}

boolean CSoftSerialDevice::Initialize (void)
{
	assert (!m_bInterruptOn);
	m_bInterruptOn = TRUE;

	m_RxDPin.ConnectInterrupt (InterruptStub, this);
	m_RxDPin.EnableInterrupt (GPIOInterruptOnFallingEdge);
	
	return TRUE;
}

int CSoftSerialDevice::Write (const void *pBuffer, size_t nCount)
{
	char *p = (char *) pBuffer;
	int nResult = 0;

	while (nCount--)
	{
		m_TxDPin.Write (LOW);
		CTimer::Get ()->usDelay (BIT_TIME);

		char chShift = *p++;
		for (unsigned nBit = 1; nBit <= 8; nBit++)
		{
			m_TxDPin.Write (chShift & 1 ? HIGH : LOW);
			CTimer::Get ()->usDelay (BIT_TIME);

			chShift >>= 1;
		}

		m_TxDPin.Write (HIGH);
		CTimer::Get ()->usDelay (BIT_TIME);

		nResult++;
	}

	return nResult;
}

int CSoftSerialDevice::Read (void *pBuffer, size_t nCount)
{
	char *p = (char *) pBuffer;
	int nResult = 0;

	while (!BufStat ())
	{
		// do nothing
	}

	while (   nCount > 0
	       && BufStat ())
	{
		char c = OutBuf ();

		*p++ = c;

		nCount--;
		nResult++;
	}

	return nResult;
}

void CSoftSerialDevice::InterruptHandler (void)
{
	assert (m_bInterruptOn);

	if (m_bStartBitExpected)
	{
		assert (m_bCurrentLevel);

		m_nLastEdgeAt = CTimer::Get ()->GetClockTicks ();

		m_RxDPin.DisableInterrupt ();
		m_RxDPin.EnableInterrupt (GPIOInterruptOnRisingEdge);

		m_bCurrentLevel = FALSE;
		m_bStartBitExpected = FALSE;
		m_nBitCount = 9;
		m_nShift = 0;

		assert (m_hTimer == 0);
		m_hTimer = CTimer::Get ()->StartKernelTimer (2, TimerStub, 0, this);
		assert (m_hTimer != 0);

		return;
	}

	unsigned nTicks = CTimer::Get()->GetClockTicks ();
	unsigned nBits = (nTicks - m_nLastEdgeAt + BIT_TIME / 2 - 1) / BIT_TIME;
	m_nLastEdgeAt = nTicks;

	if (nBits > m_nBitCount)
	{
		nBits = m_nBitCount;

		m_bStartBitExpected = TRUE;
	}

	for (unsigned nBit = 1; nBit <= nBits; nBit++)
	{
		m_nShift >>= 1;
		m_nShift |= m_bCurrentLevel ? 0x80 : 0;

		m_nBitCount--;
	}

	if (m_nBitCount == 0)
	{
		InBuf ((char) m_nShift);
		
		m_bStartBitExpected = TRUE;
	}

	if (   m_bStartBitExpected
	    || !m_bCurrentLevel)
	{
		m_RxDPin.DisableInterrupt ();
		m_RxDPin.EnableInterrupt (GPIOInterruptOnFallingEdge);
		m_bCurrentLevel = TRUE;
	}
	else
	{
		m_RxDPin.DisableInterrupt ();
		m_RxDPin.EnableInterrupt (GPIOInterruptOnRisingEdge);
		m_bCurrentLevel = FALSE;
	}

	if (   m_bStartBitExpected
	    && m_hTimer != 0)
	{
		CTimer::Get ()->CancelKernelTimer (m_hTimer);
		m_hTimer = 0;
	}
}

void CSoftSerialDevice::InterruptStub (void *pParam)
{
	CSoftSerialDevice *pThis = (CSoftSerialDevice *) pParam;
	assert (pThis != 0);

	pThis->InterruptHandler ();
}

void CSoftSerialDevice::TimerHandler (void)
{
	assert (m_bInterruptOn);
	assert (!m_bStartBitExpected);

	if (m_bCurrentLevel)
	{
		while (m_nBitCount-- > 0)
		{
			m_nShift >>= 1;
			m_nShift |= 0x80;
		}

		InBuf ((char) m_nShift);
	}

	m_RxDPin.DisableInterrupt ();
	m_RxDPin.EnableInterrupt (GPIOInterruptOnFallingEdge);
	m_bStartBitExpected = TRUE;
	m_bCurrentLevel = TRUE;
}

void CSoftSerialDevice::TimerStub (TKernelTimerHandle hTimer, void *pParam, void *pContext)
{
	CSoftSerialDevice *pThis = (CSoftSerialDevice *) pContext;
	assert (pThis != 0);

	pThis->TimerHandler ();
}

void CSoftSerialDevice::InBuf (char chChar)
{
	EnterCritical ();

	if (((m_nInPtr+1) & BUFMASK) != m_nOutPtr)
	{
		m_Buffer[m_nInPtr] = chChar;

		m_nInPtr = (m_nInPtr+1) & BUFMASK;
	}

	LeaveCritical ();
}

int CSoftSerialDevice::BufStat (void) const
{
	EnterCritical ();

	if (m_nInPtr != m_nOutPtr)
	{
		LeaveCritical ();

		return 1;
	}

	LeaveCritical ();

	return 0;
}

char CSoftSerialDevice::OutBuf (void)
{
	EnterCritical ();

	if (m_nInPtr == m_nOutPtr)
	{
		LeaveCritical ();

		return 0;
	}

	char chChar = m_Buffer[m_nOutPtr];

	m_nOutPtr = (m_nOutPtr+1) & BUFMASK;

	LeaveCritical ();

	return chChar;
}
