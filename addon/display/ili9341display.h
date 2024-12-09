//
// ili9341display.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2024  R. Stange <rsta2@o2online.de>
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
#ifndef _display_ili9341display_h
#define _display_ili9341display_h

#include <circle/display.h>
#include <circle/spimaster.h>
#include <circle/gpiopin.h>
#include <circle/types.h>

class CILI9341Display : public CDisplay /// Driver for ILI9341-based dot-matrix displays
{
public:
	static const unsigned None = GPIO_PINS;

public:
	/// \param pSPIMaster Pointer to SPI master object
	/// \param nDCPin GPIO pin number for DC pin
	/// \param nResetPin GPIO pin number for Reset pin (optional)
	/// \param nBackLightPin GPIO pin number for backlight pin (optional)
	/// \param nWidth Display width in number of pixels (default 240)
	/// \param nHeight Display height in number of pixels (default 320)
	/// \param nCPOL SPI clock polarity (0 or 1, default 0)
	/// \param nCPHA SPI clock phase (0 or 1, default 0)
	/// \param nClockSpeed SPI clock frequency in Hz
	/// \param nChipSelect SPI chip select (if connected, otherwise don't care)
	/// \param bSwapColorBytes Use big endian colors instead of normal RGB565
	/// \note GPIO pin numbers are SoC number, not header positions.
	/// \note Width/height are valid at rotation 0 (may be swapped with rotation 90 and 270).
	/// \note Big endian colors are supported by the hardware and are displayed quicker.
	CILI9341Display (CSPIMaster *pSPIMaster,
			 unsigned nDCPin, unsigned nResetPin = None, unsigned nBackLightPin = None,
			 unsigned nWidth = 240, unsigned nHeight = 320,
			 unsigned nCPOL = 0, unsigned nCPHA = 0, unsigned nClockSpeed = 15000000,
			 unsigned nChipSelect = 0, boolean bSwapColorBytes = TRUE);

	~CILI9341Display (void);

	/// \brief Set the global rotation of the display
	/// \param nDegrees Rotation in degrees counterclockwise (0, 90, 180, 270, default 0)
	/// \note Must be set before calling Initialize().
	void SetRotation (unsigned nDegrees);
	/// \return Rotation angle in degrees (0, 90, 180, 270)
	unsigned GetRotation (void) const	{ return m_nRotation; }

	/// \return Operation successful?
	boolean Initialize (void);

	/// \return Display width in number of pixels
	unsigned GetWidth (void) const		{ return m_nWidth; }
	/// \return Display height in number of pixels
	unsigned GetHeight (void) const		{ return m_nHeight; }
	/// \return Number of bits per pixels
	unsigned GetDepth (void) const		{ return 16; }

	/// \brief Set display on
	void On (void);
	/// \brief Set display off
	void Off (void);

	/// \brief Clear entire display with color
	/// \param nColor Raw color value (RGB565 or RGB565_BE, default Black)
	void Clear (TRawColor nColor = 0);

	/// \brief Set a single pixel to color
	/// \param nPosX X-position (0..width-1)
	/// \param nPosY Y-postion (0..height-1)
	/// \param nColor Raw color value (RGB565 or RGB565_BE)
	void SetPixel (unsigned nPosX, unsigned nPosY, TRawColor nColor);

	/// \brief Set area (rectangle) on the display to the raw colors in pPixels
	/// \param rArea Coordinates of the area (zero-based)
	/// \param pPixels Pointer to array with raw color values (RGB565 or RGB565_BE)
	/// \param pRoutine Routine to be called on completion
	/// \param pParam User parameter to be handed over to completion routine
	void SetArea (const TArea &rArea, const void *pPixels,
		      TAreaCompletionRoutine *pRoutine = nullptr,
		      void *pParam = nullptr);

private:
	void SetWindow (unsigned x0, unsigned y0, unsigned x1, unsigned y1);

	void SendByte (u8 uchByte, boolean bIsData);

	void Command (u8 uchByte)	{ SendByte (uchByte, FALSE); }
	void Data (u8 uchByte)		{ SendByte (uchByte, TRUE); }

	void SendData (const void *pData, size_t nLength);

	void CommandAndData (u8 uchCmd, unsigned nDataLen, ...);

private:
	CSPIMaster *m_pSPIMaster;
	unsigned m_nResetPin;
	unsigned m_nBackLightPin;
	unsigned m_nWidth;
	unsigned m_nHeight;
	unsigned m_nCPOL;
	unsigned m_nCPHA;
	unsigned m_nClockSpeed;
	unsigned m_nChipSelect;
	boolean m_bSwapColorBytes;

	u16 *m_pBuffer;

	unsigned m_nRotation;

	CGPIOPin m_DCPin;
	CGPIOPin m_ResetPin;
	CGPIOPin m_BackLightPin;
};

#endif
