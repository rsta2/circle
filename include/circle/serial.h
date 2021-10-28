//
/// \file serial.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
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

/// \class CSerialDevice
/// \brief Driver for PL011 UART
///
/// \details GPIO pin mapping (chip numbers)
/// nDevice | TXD    | RXD    | Support
/// :-----: | :----: | :----: | :------
/// 0       | GPIO14 | GPIO15 | All boards
/// ^       | GPIO32 | GPIO33 | Compute Modules
/// ^       | GPIO36 | GPIO37 | Compute Modules
/// 1       |        |        | None (AUX)
/// 2       | GPIO0  | GPIO1  | Raspberry Pi 4 only
/// 3       | GPIO4  | GPIO5  | Raspberry Pi 4 only
/// 4       | GPIO8  | GPIO9  | Raspberry Pi 4 only
/// 5       | GPIO12 | GPIO13 | Raspberry Pi 4 only
/// GPIO32/33 and GPIO36/37 can be selected with system option SERIAL_GPIO_SELECT.\n
/// GPIO0/1 are normally reserved for ID EEPROM.\n
/// Handshake lines CTS and RTS are not supported.

#if RASPPI < 4
	#define SERIAL_DEVICES		1
#else
	#define SERIAL_DEVICES		6
#endif

#define SERIAL_BUF_SIZE		2048			// must be a power of 2
#define SERIAL_BUF_MASK		(SERIAL_BUF_SIZE-1)

// serial options
#define SERIAL_OPTION_ONLCR	(1 << 0)	///< Translate NL to NL+CR on output (default)

// returned from Read/Write as negative value
#define SERIAL_ERROR_BREAK	1
#define SERIAL_ERROR_OVERRUN	2
#define SERIAL_ERROR_FRAMING	3
#define SERIAL_ERROR_PARITY	4

class CSerialDevice : public CDevice
{
public:
	enum TParity
	{
		ParityNone,
		ParityOdd,
		ParityEven,
		ParityUnknown
	};

public:
#ifndef USE_RPI_STUB_AT
	/// \param pInterruptSystem Pointer to interrupt system object (or 0 for polling driver)
	/// \param bUseFIQ Use FIQ instead of IRQ
	/// \param nDevice Device number (see: GPIO pin mapping)
	CSerialDevice (CInterruptSystem *pInterruptSystem = 0, boolean bUseFIQ = FALSE,
		       unsigned nDevice = 0);

	~CSerialDevice (void);
#endif

	/// \param nBaudrate Baud rate in bits per second
	/// \param nDataBits Number of data bits (5..8, default 8)
	/// \param nStopBits Number of stop bits (1..2, default 1)
	/// \param Parity Parity setting (ParityNone (default), ParityOdd or ParityEven)
	/// \return Operation successful?
#ifndef USE_RPI_STUB_AT
	boolean Initialize (unsigned nBaudrate = 115200,
			    unsigned nDataBits = 8, unsigned nStopBits = 1,
			    TParity Parity = ParityNone);
#else
	boolean Initialize (unsigned nBaudrate = 115200);
#endif

	/// \param pBuffer Pointer to data to be sent
	/// \param nCount Number of bytes to be sent
	/// \return Number of bytes successfully sent (< 0 on error)
	int Write (const void *pBuffer, size_t nCount);

#ifndef USE_RPI_STUB_AT
	/// \param pBuffer Pointer to buffer for received data
	/// \param nCount Maximum number of bytes to be received
	/// \return Number of bytes received (0 no data available, < 0 on error)
	int Read (void *pBuffer, size_t nCount);

	/// \return Serial options mask (see serial options)
	unsigned GetOptions (void) const;
	/// \param nOptions Serial options mask (see serial options)
	void SetOptions (unsigned nOptions);

	typedef void TMagicReceivedHandler (void);
	/// \param pMagic String for which is searched in the received data\n
	/// (must remain valid after return from this method)
	/// \param pHandler Handler which is called, when the magic string is found
	/// \note Does only work with interrupt driver.
	void RegisterMagicReceivedHandler (const char *pMagic, TMagicReceivedHandler *pHandler);

protected:
	/// \return Number of bytes buffer space available for Write()
	/// \note Does only work with interrupt driver.
	unsigned AvailableForWrite (void);

	/// \return Number of bytes already received available for Read()
	/// \note Does only work with interrupt driver.
	unsigned AvailableForRead (void);

	/// \return Next received byte which will be returned by Read() (-1 if no data available)
	/// \note Does only work with interrupt driver.
	int Peek (void);

	/// \brief Waits until all written bytes have been sent out
	void Flush (void);

private:
	boolean Write (u8 uchChar);

	void InterruptHandler (void);
	static void InterruptStub (void *pParam);

private:
	CInterruptSystem *m_pInterruptSystem;
	boolean m_bUseFIQ;
	unsigned m_nDevice;
	uintptr  m_nBaseAddress;
	boolean  m_bValid;

#if SERIAL_GPIO_SELECT == 14
	CGPIOPin m_GPIO32;
	CGPIOPin m_GPIO33;
#endif
	CGPIOPin m_TxDPin;
	CGPIOPin m_RxDPin;

	u8 m_RxBuffer[SERIAL_BUF_SIZE];
	volatile unsigned m_nRxInPtr;
	volatile unsigned m_nRxOutPtr;
	volatile int m_nRxStatus;

	u8 m_TxBuffer[SERIAL_BUF_SIZE];
	volatile unsigned m_nTxInPtr;
	volatile unsigned m_nTxOutPtr;

	unsigned m_nOptions;

	const char *m_pMagic;
	const char *m_pMagicPtr;
	TMagicReceivedHandler *m_pMagicReceivedHandler;

	CSpinLock m_SpinLock;
	CSpinLock m_LineSpinLock;

	static unsigned s_nInterruptUseCount;
	static CInterruptSystem *s_pInterruptSystem;
	static boolean s_bUseFIQ;
	static volatile u32 s_nInterruptDeviceMask;
	static CSerialDevice *s_pThis[SERIAL_DEVICES];
#endif
};

#endif
