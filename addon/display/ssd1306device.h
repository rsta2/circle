//
// ssd1306device.h
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
#ifndef _display_ssd1306device_h
#define _display_ssd1306device_h

#include <circle/device.h>
#include <circle/i2cmaster.h>
#include <circle/spinlock.h>
#include <circle/types.h>
#include "chardevice.h"

// Hard-codes how the text maps to the graphics display
#define SSD1306_COLUMNS	20	// 128/6 ~= 22
#define SSD1306_ROWS	2	// 32/8   = 4; But code uses Double Height Font

class CSSD1306Device : public CCharDevice	/// LCD dot-matrix display driver (using SSD1306 controller)
{
public:
	/// \param nWidth Display size in pixels (128 only)
	/// \param nHeight Display size in pixels (32 or 64)
	/// \param pI2CMaster I2C master to be used
	/// \param nAddress I2C slave address of the display controller
	/// \param rotated Display rotated?
	/// \param mirrored Display mirrored?
	CSSD1306Device (unsigned nWidth, unsigned nHeight,
			CI2CMaster *pI2CMaster, u8 nAddress,
			bool rotated=false, bool mirrored=false);
	~CSSD1306Device (void);

	/// \return Operation successful?
	boolean Initialize (void);

private:
	void DevClearCursor (void) override;
	void DevSetCursor (unsigned nCursorX, unsigned nCursorY) override;
	void DevSetCursorMode (boolean bVisible) override;
	void DevSetChar (unsigned nPosX, unsigned nPosY, char chChar) override;
	void DevUpdateDisplay (void) override;

	// Character functions
	void Clear(bool bImmediate = false);
	void Print(const char* pText, u8 nCursorX, u8 nCursorY, bool bClearLine = false, bool bImmediate = false);

	// Graphics functions
	void DrawChar(char chChar, u8 nCursorX, u8 nCursorY, bool bInverted = false, bool bDoubleWidth = false);
	void Flip();

	void SetBacklightState(bool bEnabled);

private:
	unsigned m_nWidth;
	unsigned m_nHeight;
	CI2CMaster *m_pI2CMaster;
	u8 m_nAddress;
	bool m_bBacklightEnabled;
    bool m_bRotated;
    bool m_bMirrored;

	struct TFrameBufferUpdatePacket
	{
		u8 DataControlByte;
		u8 FrameBuffer[128 * 64 / 8];
	}
	PACKED;

	void WriteCommand(u8 nCommand) const;
	void WriteFrameBuffer(bool bForceFullUpdate = false) const;
	void SwapFrameBuffers();

	// Double framebuffers
	TFrameBufferUpdatePacket m_FrameBuffers[2];
	u8 m_nCurrentFrameBuffer;
};

#endif
