//
// btuarttransport.h
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
#ifndef _circle_bt_btuarttransport_h
#define _circle_bt_btuarttransport_h

#include <circle/device.h>
#include <circle/bt/bttransportlayer.h>
#include <circle/bt/bluetooth.h>
#include <circle/interrupt.h>
#include <circle/gpiopin.h>
#include <circle/types.h>

class CBTUARTTransport : public CDevice
{
public:
	CBTUARTTransport (CInterruptSystem *pInterruptSystem);
	~CBTUARTTransport (void);

	boolean Initialize (unsigned nBaudrate = 115200);

	boolean SendHCICommand (const void *pBuffer, unsigned nLength);

	void RegisterHCIEventHandler (TBTHCIEventHandler *pHandler);

private:
	void Write (u8 nChar);

	void IRQHandler (void);
	static void IRQStub (void *pParam);

private:
	CGPIOPin m_GPIO14;
	CGPIOPin m_GPIO15;
	CGPIOPin m_TxDPin;
	CGPIOPin m_RxDPin;

	CInterruptSystem *m_pInterruptSystem;
	boolean m_bIRQConnected;

	TBTHCIEventHandler *m_pEventHandler;

	u8 m_RxBuffer[BT_MAX_HCI_EVENT_SIZE];
	unsigned m_nRxState;
	unsigned m_nRxParamLength;
	unsigned m_nRxInPtr;
};

#endif
