//
// mphi.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2022  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_mphi_h
#define _circle_mphi_h

#include <circle/interrupt.h>
#include <circle/types.h>

class CMPHIDevice	/// A driver, which uses the MPHI device to generate an IRQ
{
public:
	CMPHIDevice (CInterruptSystem *pInterrupt);
	~CMPHIDevice (void);

	void ConnectHandler (TIRQHandler *pHandler, void *pParam = nullptr);
	void DisconnectHandler (void);

	void TriggerIRQ (void);

private:
	static void IRQHandler (void *pParam);

private:
	CInterruptSystem *m_pInterrupt;

	TIRQHandler *m_pHandler;
	void *m_pParam;

	u8 *m_pDummyDMABuffer;

	volatile int m_nTriggerCount;
	int m_nIntCount;
};

#endif
