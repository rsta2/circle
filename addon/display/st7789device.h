//
// st7789device.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2018-2024  R. Stange <rsta2@o2online.de>
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
#ifndef _display_st7789device_h
#define _display_st7789device_h

#include <circle/device.h>
#include <circle/gpiopin.h>
#include <circle/spinlock.h>
#include <circle/types.h>
#include "chardevice.h"
#include "st7789display.h"

class CST7789Device : public CCharDevice	/// LCD dot-matrix display driver (using ST7789 controller)
{
public:
	/// \param pSPIMaster
	/// \param pST7789Display
	/// \param nColumns Display size in number of columns (max. 40)
	/// \param nRows    Display size in number of rows (max. 4)
	/// \param bDoubleWidth Use thicker characters on screen
	/// \param bDoubleHeight Use higher characters on screen
	/// \param bBlockCursor Use blinking block cursor instead of underline cursor
	CST7789Device (CSPIMaster *pSPIMaster, CST7789Display *pST7789Display,
		unsigned nColumns, unsigned nRows, bool bDoubleWidth = TRUE, bool bDoubleHeight = TRUE,
		boolean bBlockCursor = FALSE);

	~CST7789Device (void);

	/// \return Operation successful?
	boolean Initialize (void);

private:
	void DevClearCursor (void) override;
	void DevSetCursor (unsigned nCursorX, unsigned nCursorY) override;
	void DevSetCursorMode (boolean bVisible) override;
	void DevSetChar (unsigned nPosX, unsigned nPosY, char chChar) override;
	void DevUpdateDisplay (void) override;

private:
	CSPIMaster		*m_pSPIMaster;
	CST7789Display	*m_pST7789Display;

	unsigned m_nColumns;
	unsigned m_nRows;
	unsigned m_nCharW;
	unsigned m_nCharH;
	bool     m_bDoubleWidth;
	bool     m_bDoubleHeight;

	boolean m_bBlockCursor;
};

#endif
