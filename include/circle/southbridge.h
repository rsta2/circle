//
// southbridge.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2023  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_southbridge_h
#define _circle_southbridge_h

#if RASPPI >= 5

#include <circle/rp1int.h>
#include <circle/interrupt.h>
#include <circle/bcmpciehostbridge.h>
#include <circle/types.h>

class CSouthbridge	/// Driver for the RP1 multi-function device of the Raspberry Pi 5
{
public:
	/// \param pInterrupt Pointer to the interrupt system object
	CSouthbridge (CInterruptSystem *pInterrupt);

	~CSouthbridge (void);

	/// \return Operation successful?
	boolean Initialize (void);

	/// \brief Connect to second level IRQ at RP1
	/// \param nIRQ IRQ number to connect
	/// \param pHandler Pointer to IRQ handler
	/// \param pParam User parameter to be handed over to handler
	void ConnectIRQ (unsigned nIRQ, TIRQHandler *pHandler, void *pParam);
	/// \brief Disconnect from second level IRQ at RP1
	void DisconnectIRQ (unsigned nIRQ);

	/// \param nIRQ IRQ number to enable
	static void EnableIRQ (unsigned nIRQ);
	/// \param nIRQ IRQ number to disable
	static void DisableIRQ (unsigned nIRQ);

	/// \return Pointer to the only RP1 object
	static CSouthbridge *Get (void);

	/// \return Has RP1 already been initialized?
	static boolean IsInitialized (void)
	{
		return s_bIsInitialized;
	}

#ifndef NDEBUG
	void DumpStatus (void);
#endif

private:
	static void InterruptHandler (void *pParam);

private:
	CInterruptSystem *m_pInterrupt;

	CBcmPCIeHostBridge *m_pPCIe;

	u64 m_ulEnableMask;
	TIRQHandler *m_apIRQHandler[RP1_IRQ_LINES];
	void *m_pParam[RP1_IRQ_LINES];
	boolean m_bEdgeTriggered[RP1_IRQ_LINES];

	static boolean s_bIsInitialized;

	static CSouthbridge *s_pThis;
};

#endif

#endif
