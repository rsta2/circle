//
// interrupt.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2019  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_interrupt_h
#define _circle_interrupt_h

#include <circle/bcm2835int.h>
#include <circle/exceptionstub.h>
#include <circle/types.h>

typedef void TIRQHandler (void *pParam);

class CInterruptSystem
{
public:
	CInterruptSystem (void);
	~CInterruptSystem (void);

	boolean Initialize (void);

	void ConnectIRQ (unsigned nIRQ, TIRQHandler *pHandler, void *pParam);
	void DisconnectIRQ (unsigned nIRQ);

	void ConnectFIQ (unsigned nFIQ, TFIQHandler *pHandler, void *pParam);
	void DisconnectFIQ (void);

	static void EnableIRQ (unsigned nIRQ);
	static void DisableIRQ (unsigned nIRQ);

	static void EnableFIQ (unsigned nFIQ);
	static void DisableFIQ (void);

	static CInterruptSystem *Get (void);

	static void InterruptHandler (void);

#if RASPPI >= 4
	static void InitializeSecondary (void);

	static void SendIPI (unsigned nCore, unsigned nIPI);

	static void CallSecureMonitor (u32 nFunction, u32 nParam);
	static void SecureMonitorHandler (u32 nFunction, u32 nParam);
#endif

private:
	boolean CallIRQHandler (unsigned nIRQ);

private:
	TIRQHandler	*m_apIRQHandler[IRQ_LINES];
	void		*m_pParam[IRQ_LINES];

	static CInterruptSystem *s_pThis;
};

#endif
