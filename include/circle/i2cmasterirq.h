//
/// \file i2cmasterirq.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2024  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_i2cmasterirq_h
#define _circle_i2cmasterirq_h

#include <circle/gpiopin.h>
#include <circle/interrupt.h>
#include <circle/spinlock.h>
#include <circle/types.h>

/// \class CI2CMasterIRQ
/// \brief Driver for I2C master devices - async using IRQ
///
/// \details GPIO pin mapping
/// nDevice   | nConfig 0     | nConfig 1     | nConfig 2     | Boards
/// :-------: | :-----------: | :-----------: | :-----------: | :-----
/// ^         | SDA    SCL    | SDA    SCL    | SDA    SCL    | ^
/// 0         | GPIO0  GPIO1  | GPIO28 GPIO29 | GPIO44 GPIO45 | Rev. 1, other
/// 1         | GPIO2  GPIO3  |               |               | All other
/// 2         |               |               |               | None
/// 3         | GPIO2  GPIO3  | GPIO4  GPIO5  |               | Raspberry Pi 4 only
/// 4         | GPIO6  GPIO7  | GPIO8  GPIO9  |               | Raspberry Pi 4 only
/// 5         | GPIO10 GPIO11 | GPIO12 GPIO13 |               | Raspberry Pi 4 only
/// 6         | GPIO22 GPIO23 |               |               | Raspberry Pi 4 only


/// \param nStatus   One of the CI2CMasterIRQ::TStatus integers below
/// \param pParam    The pParam passed in CI2CMasterIRQ::SetCompletionRoutine()
typedef void TI2CCompletionRoutine (int nStatus, void *pParam);


class CI2CMasterIRQ
{
public:
	enum TStatus
	{
		StatusClockStretchTimeout = -5,
		StatusDataLeftToReadError = -4,
		StatusAckError = -3,
		StatusInvalidParam = -2,
		StatusInvalidState = -1,
		StatusSuccess = 0,
		StatusWriting = 1,
		StatusReading = 2,
	};

public:
	/// \param pInterruptSystem Pointer to the interrupt system object
	/// \param nDevice   Device number (see: GPIO pin mapping)
	/// \param bFastMode Use I2C fast mode (400 KHz) or standard mode (100 KHz) otherwise
	/// \param nConfig   GPIO mapping configuration (see: GPIO pin mapping)
	CI2CMasterIRQ (CInterruptSystem *pInterruptSystem, unsigned nDevice, boolean bFastMode = FALSE, unsigned nConfig = 0);

	~CI2CMasterIRQ (void);

	/// \return Initialization successful?
	boolean Initialize (void);

	/// \brief Modify default clock before specific transfer
	/// \param nClockSpeed I2C clock frequency in Hz
	void SetClock (unsigned nClockSpeed);

	/// \brief Sets the completion routine called when the Read/Write/StartWriteRead functions complete
	void SetCompletionRoutine (TI2CCompletionRoutine *pRoutine, void *pParam = 0);

	/// \return one of the TStatus integer above
	int GetStatus();

	/// \param ucAddress I2C slave address of target device
	/// \param pBuffer   Read data will be stored here
	/// \param nCount    Number of bytes to be read
	/// \return 0 if success or < 0 on failure
	int Read (u8 ucAddress, void *pBuffer, unsigned nCount);

	/// \param ucAddress I2C slave address of target device
	/// \param pBuffer   Write data for will be taken from here
	/// \param nCount    Number of bytes to be written
	/// \return 0 if success or < 0 on failure
	int Write (u8 ucAddress, const void *pBuffer, unsigned nCount);

	/// \brief Consecutive write and read operation with potential asynchronous callback (non-blocking)
	/// \param ucAddress    I2C slave address of target device
	/// \param pWriteBuffer Write data for will be taken from here
	/// \param nWriteCount  Number of bytes to be written (max. 16)
	/// \param pReadBuffer  Read data will be stored here
	/// \param nReadCount   Number of bytes to be read
	/// \return 0 if success or < 0 on failure
	int StartWriteRead (u8 ucAddress,
			    const void *pWriteBuffer, unsigned nWriteCount,
			    void *pReadBuffer, unsigned nReadCount);

private:
	void InterruptHandler (void);
	static void InterruptStub (void *pParam);

	int StartTransfer (u8 ucAddress,
			   const void *pWriteBuffer, unsigned nWriteCount,
			   void *pReadBuffer, unsigned nReadCount,
			   bool bValidateReadBuffer, bool bValidateWriteBuffer);
	void CallCompletionRoutine();

private:
	unsigned m_nDevice;
	uintptr  m_nBaseAddress;
	boolean  m_bFastMode;
	unsigned m_nConfig;
	boolean  m_bValid;

	CGPIOPin m_SDA;
	CGPIOPin m_SCL;

	unsigned m_nCoreClockRate;
	unsigned m_nClockSpeed;

	CSpinLock m_SpinLock;

	int m_nStatus;
	void *m_pReadBuffer;
	unsigned m_nReadCount;

	CInterruptSystem *m_pInterruptSystem;
	TI2CCompletionRoutine *m_pCompletionRoutine;
	void *m_pCompletionParam;
};

#endif
