//
// usertimer.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2018  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usertimer_h
#define _circle_usertimer_h

#include <circle/interrupt.h>
#include <circle/types.h>

class CUserTimer;

typedef void TUserTimerHandler (CUserTimer *pUserTimer, void *pParam);

class CUserTimer	/// Fine grained user programmable interrupt timer (based on ARM_IRQ_TIMER1)
{
public:
	/// \param pInterruptSystem Pointer to the interrupt system object
	/// \param pHandler Pointer to the user timer handler
	/// \param pParam Parameter handed over to the user timer handler
	/// \param bUseFIQ Use FIQ instead of IRQ
	CUserTimer (CInterruptSystem *pInterruptSystem,
		    TUserTimerHandler *pHandler, void *pParam = 0,
		    boolean bUseFIQ = FALSE);

	~CUserTimer (void);

	/// \return Operation successful?
	/// \note Automatically starts the timer with a delay of 1 hour
	boolean Initialize (void);

	/// \brief Stops the user timer, has to be re-initialized to be used again
	void Stop (void);

	/// \param nDelayMicros User timer elapses after this number of microseconds (> 1)
	/// \note Must be called from user timer handler to a set new delay!
	/// \note Can be called on a running user timer with a new delay
	void Start (unsigned nDelayMicros);
#define USER_CLOCKHZ	1000000U

private:
	static void InterruptHandler (void *pParam);

private:
	CInterruptSystem  *m_pInterruptSystem;
	TUserTimerHandler *m_pHandler;
	void		  *m_pParam;
	boolean		   m_bUseFIQ;

	boolean m_bInitialized;
};

#endif
