//
// serial.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2017  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_serial_h
#define _circle_serial_h

#include <circle/device.h>
#include <circle/interrupt.h>
#include <circle/gpiopin.h>
#include <circle/spinlock.h>
#include <circle/sysconfig.h>
#include <circle/types.h>

#define SERIAL_BUF_SIZE		2048			// must be a power of 2
#define SERIAL_BUF_MASK		(SERIAL_BUF_SIZE-1)

// serial options
#define SERIAL_OPTION_ONLCR	(1 << 0)		// translate NL to NL+CR on output (default)

// returned from Read/Write as negative value
#define SERIAL_ERROR_BREAK	1
#define SERIAL_ERROR_OVERRUN	2
#define SERIAL_ERROR_FRAMING	3

class CSerialDevice : public CDevice		/// Driver for PL011 UART0 at GPIO14/15
{
public:
#ifndef USE_RPI_STUB_AT
	/// \param pInterruptSystem Pointer to interrupt system object (or 0 for polling driver)
	/// \param bUseFIQ Use FIQ instead of IRQ
	CSerialDevice (CInterruptSystem *pInterruptSystem = 0, boolean bUseFIQ = FALSE);

	~CSerialDevice (void);
#endif

	/// \param nBaudrate Baud rate in bits per second
	/// \return Operation successful?
	boolean Initialize (unsigned nBaudrate = 115200);

	/// \param pBuffer Pointer to data to be sent
	/// \param nCount Number of bytes to be sent
	/// \return Number of bytes successfully sent (< 0 on error)
	int Write (const void *pBuffer, unsigned nCount);

#ifndef USE_RPI_STUB_AT
	/// \param pBuffer Pointer to buffer for received data
	/// \param nCount Maximum number of bytes to be received
	/// \return Number of bytes received (0 no data available, < 0 on error)
	int Read (void *pBuffer, unsigned nCount);

	/// \return Serial options mask (see serial options)
	unsigned GetOptions (void) const;
	/// \param nOptions Serial options mask (see serial options)
	void SetOptions (unsigned nOptions);

private:
	boolean Write (u8 uchChar);

	void InterruptHandler (void);
	static void InterruptStub (void *pParam);

private:
	CGPIOPin m_GPIO32;
	CGPIOPin m_GPIO33;
	CGPIOPin m_TxDPin;
	CGPIOPin m_RxDPin;

	CInterruptSystem *m_pInterruptSystem;
	boolean m_bUseFIQ;
	boolean m_bInterruptConnected;

	u8 m_RxBuffer[SERIAL_BUF_SIZE];
	volatile unsigned m_nRxInPtr;
	volatile unsigned m_nRxOutPtr;
	volatile int m_nRxStatus;

	u8 m_TxBuffer[SERIAL_BUF_SIZE];
	volatile unsigned m_nTxInPtr;
	volatile unsigned m_nTxOutPtr;

	unsigned m_nOptions;

	CSpinLock m_SpinLock;
	CSpinLock m_LineSpinLock;
#endif
};

#endif
