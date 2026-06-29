//
// sh1106device.h
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
#ifndef _display_sh1106device_h
#define _display_sh1106device_h

#include <circle/device.h>
#include <circle/i2cmaster.h>
#include <circle/spinlock.h>
#include <circle/types.h>
#include <display/chardevice.h>

// Hard-codes how the text maps to the graphics display
#define SH1106_COLUMNS	20	// 128/6 ~= 22
#define SH1106_ROWS	2	// 32/8   = 4; But code uses Double Height Font

class CSH1106Device : public CCharDevice	/// LCD dot-matrix display driver (using SH1106 controller)
{
public:
	/// \param nWidth Display size in pixels (128 only)
	/// \param nHeight Display size in pixels (32 or 64)
	/// \param pI2CMaster I2C master to be used
	/// \param nAddress I2C slave address of the display controller
	/// \param nRotate Display rotation in degrees (0, 90, 180, or 270)
	/// \param mirrored Display mirrored?
	CSH1106Device (unsigned nWidth, unsigned nHeight,
		       CI2CMaster *pI2CMaster, u8 nAddress,
		       unsigned nRotate=0, bool mirrored=false);
	~CSH1106Device (void);

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
	unsigned m_nRotate;
	bool m_bMirrored;

	struct TFrameBufferUpdatePacket
	{
		u8 DataControlByte;
		u8 FrameBuffer[128 * 64 / 8];
	}
	PACKED;

	void WriteCommand(u8 nCommand) const;
	void PutPixel(u8 *pFrameBuffer, unsigned nX, unsigned nY, bool bSet) const;
	void PutRotatedPixel(u8 *pFrameBuffer, unsigned nX, unsigned nY, bool bSet) const;
	void PutColumn(u8 *pFrameBuffer, unsigned nX, unsigned nPageY, u8 nColumn) const;
	void WriteFrameBuffer(bool bForceFullUpdate = false) const;
	void SwapFrameBuffers();

	// Double framebuffers
	TFrameBufferUpdatePacket m_FrameBuffers[2];
	u8 m_nCurrentFrameBuffer;
};

#endif
