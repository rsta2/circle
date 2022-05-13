//
// hd44780device.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2018-2022  R. Stange <rsta2@o2online.de>
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
#ifndef _display_hd44780device_h
#define _display_hd44780device_h

#include <circle/device.h>
#include <circle/gpiopin.h>
#include <circle/spinlock.h>
#include <circle/types.h>
#include <circle/i2cmaster.h>
#include "chardevice.h"

#define HD44780_CMD  0
#define HD44780_DATA 1

class CHD44780Device : public CCharDevice	/// LCD dot-matrix display driver (using HD44780 controller)
{
public:
	/// \param nColumns Display size in number of columns (max. 40)
	/// \param nRows    Display size in number of rows (max. 4)
	/// \param nD4Pin   GPIO pin number of Data 4 pin (Brcm numbering)
	/// \param nD5Pin   GPIO pin number of Data 5 pin (Brcm numbering)
	/// \param nD6Pin   GPIO pin number of Data 6 pin (Brcm numbering)
	/// \param nD7Pin   GPIO pin number of Data 7 pin (Brcm numbering)
	/// \param nENPin   GPIO pin number of Enable pin (Brcm numbering)
	/// \param nRSPin   GPIO pin number of Register Select pin (Brcm numbering)
	/// \param nRWPin   GPIO pin number of Read/Write pin (Brcm numbering, 0 if not connected)
	/// \param bBlockCursor Use blinking block cursor instead of underline cursor
	/// \note Driver uses 4-bit mode, pins D0-D3 are not used.
	CHD44780Device (unsigned nColumns, unsigned nRows,
			unsigned nD4Pin, unsigned nD5Pin, unsigned nD6Pin, unsigned nD7Pin,
			unsigned nENPin, unsigned nRSPin, unsigned nRWPin = 0,
			boolean bBlockCursor = FALSE);

	/// \param pI2CMaster Pointer to I2C master object
	/// \param nAddress   I2C slave address of display
	/// \param nColumns   Display size in number of columns (max. 40)
	/// \param nRows      Display size in number of rows (max. 4)
	/// \param bBlockCursor Use blinking block cursor instead of underline cursor
	CHD44780Device (CI2CMaster *pI2CMaster, u8 nAddress,
			unsigned nColumns, unsigned nRows, boolean bBlockCursor = FALSE);

	~CHD44780Device (void);

	/// \return Operation successful?
	boolean Initialize (void);

	/// \brief Define the 5x7 font for the definable characters
	/// \param chChar   Character code (0x80..0x87)
	/// \param FontData Font bit map for character pixel line 0-7 (only bits 4-0 are used)
	/// \note Line 7 is reserved for the cursor and is usually 0x00
	void DefineCharFont (char chChar, const u8 FontData[8]);

private:
	void DevClearCursor (void) override;
	void DevSetCursor (unsigned nCursorX, unsigned nCursorY) override;
	void DevSetCursorMode (boolean bVisible) override;
	void DevSetChar (unsigned nPosX, unsigned nPosY, char chChar) override;
	void DevUpdateDisplay (void) override;

	void WriteByte (u8 nData, int mode);
	void WriteHalfByte (u8 nData, int mode);

	u8 GetDDRAMAddress (unsigned nPosX, unsigned nPosY);

private:
	CGPIOPin  m_D4;
	CGPIOPin  m_D5;
	CGPIOPin  m_D6;
	CGPIOPin  m_D7;
	CGPIOPin  m_EN;
	CGPIOPin  m_RS;
	CGPIOPin *m_pRW;

	CI2CMaster *m_pI2CMaster;
	u8 m_nAddress;

	boolean m_bBlockCursor;
};

#endif
