//
// softserial.h
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
#ifndef _softserial_h
#define _softserial_h

#include <circle/device.h>
#include <circle/gpiomanager.h>
#include <circle/gpiopin.h>
#include <circle/timer.h>

#define SOFTSERIAL_BUFSIZE	64

class CSoftSerialDevice : public CDevice
{
public:
	CSoftSerialDevice (unsigned nGPIOPinTxD, unsigned nGPIOPinRxD, CGPIOManager *pGPIOManager);
	~CSoftSerialDevice (void);

	boolean Initialize (void);

	int Write (const void *pBuffer, size_t nCount);
	int Read (void *pBuffer, size_t nCount);

private:
	void InterruptHandler (void);
	static void InterruptStub (void *pParam);

	void TimerHandler (void);
	static void TimerStub (TKernelTimerHandle hTimer, void *pParam, void *pContext);

	void InBuf (char chChar);
	int BufStat (void) const;
	char OutBuf (void);		// returns '\0' if no key is waiting
	
private:
	CGPIOPin m_TxDPin;
	CGPIOPin m_RxDPin;
	boolean  m_bInterruptOn;

	TKernelTimerHandle m_hTimer;

	boolean  m_bCurrentLevel;
	boolean  m_bStartBitExpected;
	unsigned m_nLastEdgeAt;
	unsigned m_nBitCount;
	unsigned m_nShift;

	char     m_Buffer[SOFTSERIAL_BUFSIZE];
	unsigned m_nInPtr;
	unsigned m_nOutPtr;
};

#endif
