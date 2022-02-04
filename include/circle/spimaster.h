//
/// \file spimaster.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2022  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_spimaster_h
#define _circle_spimaster_h

#include <circle/gpiopin.h>
#include <circle/spinlock.h>
#include <circle/types.h>

/// \class CSPIMaster
/// \brief Driver for SPI (non-AUX) master devices. Synchronous polling operation.
///
/// \details Supported features
/// - SPI non-AUX devices only
/// - Standard mode (3-wire) only
/// - Chip select lines (CE0, CE1) are active low
/// - Polled operation only
///
/// \details GPIO pin mapping (chip numbers)
/// nDevice | MISO   | MOSI   | SCLK   | CE0    | CE1    | Support
/// :-----: | :----: | :----: | :----: | :----: | :----: | :------
/// 0       | GPIO9  | GPIO10 | GPIO11 | GPIO8  | GPIO7  | All boards
/// 1       |        |        |        |        |        | class CSPIMasterAUX
/// 2       |        |        |        |        |        | None
/// 3       | GPIO1  | GPIO2  | GPIO3  | GPIO0  | GPIO24 | Raspberry Pi 4 only
/// 4       | GPIO5  | GPIO6  | GPIO7  | GPIO4  | GPIO25 | Raspberry Pi 4 only
/// 5       | GPIO13 | GPIO14 | GPIO15 | GPIO12 | GPIO26 | Raspberry Pi 4 only
/// 6       | GPIO19 | GPIO20 | GPIO21 | GPIO18 | GPIO27 | Raspberry Pi 4 only
/// GPIO0/1 are normally reserved for ID EEPROM.

class CSPIMaster
{
public:
	const unsigned ChipSelectNone = 3;

public:
	/// \param nClockSpeed SPI clock frequency in Hz
	/// \param CPOL        Clock polarity
	/// \param CPHA        Clock phase
	/// \param nDevice     Device number (see: GPIO pin mapping)
	CSPIMaster (unsigned nClockSpeed = 500000, unsigned CPOL = 0, unsigned CPHA = 0,
		    unsigned nDevice = 0);

	~CSPIMaster (void);

	/// \return Initialization successful?
	boolean Initialize (void);

	/// \brief Modify default SPI clock frequency before specific transfer
	/// \param nClockSpeed SPI clock frequency in Hz
	/// \note Not protected by internal spinlock for multi-core operation
	void SetClock (unsigned nClockSpeed);
	/// \brief Modify default clock polarity/phase before specific transfer
	/// \param CPOL Clock polarity
	/// \param CPHA Clock phase
	/// \note Not protected by internal spinlock for multi-core operation
	void SetMode (unsigned CPOL, unsigned CPHA);

	/// \param nMicroSeconds Additional time, CE# stays active (for the next transfer only)
	/// \note Normally chip select goes inactive very soon after transfer,
	///       this sets the additional time, CE# stays active
	void SetCSHoldTime (unsigned nMicroSeconds);

	/// \param nChipSelect CE# to be used (0, 1 or ChipSelectNone)
	/// \param pBuffer     Read data will be stored here
	/// \param nCount      Number of bytes to be read
	/// \return Number of read bytes or < 0 on failure
	int Read (unsigned nChipSelect, void *pBuffer, unsigned nCount);

	/// \param nChipSelect CE# to be used (0, 1 or ChipSelectNone)
	/// \param pBuffer     Write data for will be taken from here
	/// \param nCount      Number of bytes to be written
	/// \return Number of written bytes or < 0 on failure
	int Write (unsigned nChipSelect, const void *pBuffer, unsigned nCount);

	/// \param nChipSelect CE# to be used (0, 1 or ChipSelectNone)
	/// \param pWriteBuffer Write data for will be taken from here
	/// \param pReadBuffer  Read data will be stored here
	/// \param nCount       Number of bytes to be transferred
	/// \return Number of bytes transferred or < 0 on failure
	int WriteRead (unsigned nChipSelect, const void *pWriteBuffer, void *pReadBuffer,
		       unsigned nCount);

private:
	unsigned m_nClockSpeed;
	unsigned m_CPOL;
	unsigned m_CPHA;
	unsigned m_nDevice;
	uintptr  m_nBaseAddress;
	boolean  m_bValid;

	CGPIOPin m_SCLK;
	CGPIOPin m_MOSI;
	CGPIOPin m_MISO;
	CGPIOPin m_CE0;
	CGPIOPin m_CE1;

	unsigned m_nCoreClockRate;

	unsigned m_nCSHoldTime;

	CSpinLock m_SpinLock;
};

#endif
