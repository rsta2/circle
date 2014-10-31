//
// interrupt.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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
#ifndef _interrupt_h
#define _interrupt_h

#include <circle/bcm2835int.h>
#include <circle/exceptionstub.h>

typedef void TIRQHandler (void *pParam);

class CInterruptSystem
{
public:
	CInterruptSystem (void);
	~CInterruptSystem ();

	int Initialize (void);

	void ConnectIRQ (unsigned nIRQ, TIRQHandler *pHandler, void *pParam);
	void DisconnectIRQ (unsigned nIRQ);

	static void EnableIRQ (unsigned nIRQ);
	static void DisableIRQ (unsigned nIRQ);

	static void InterruptHandler (void);

private:
	int CallIRQHandler (unsigned nIRQ);

private:
	TIRQHandler	*m_apIRQHandler[IRQ_LINES];
	void		*m_pParam[IRQ_LINES];

	static CInterruptSystem *s_pThis;
};

#endif
