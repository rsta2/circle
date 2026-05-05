//
// ssd1309display.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2025  R. Stange <rsta2@o2online.de>
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
#ifndef _display_ssd1309display_h
#define _display_ssd1309display_h

#include <circle/display.h>
#include <circle/i2cmaster.h>
#include <circle/spimaster.h>
#include <circle/gpiopin.h>
#include <circle/types.h>

class CSSD1309Display : public CDisplay	/// Driver for SSD1309-based OLED dot-matrix displays
{
public:
	static const unsigned Width = 128;
	static const unsigned Height = 64;

public:
	/// \brief Constructor for I2C interface
	/// \param pI2CMaster I2C master to be used
	/// \param nResetPin GPIO pin number for Reset pin (active low)
	/// \param uchI2CAddress I2C slave address of the display controller (default 0x3C)
	/// \param nClockSpeed I2C clock frequency in Hz (or 0 for system default)
	CSSD1309Display (CI2CMaster *pI2CMaster,
			 unsigned nResetPin,
			 u8 uchI2CAddress = 0x3C, unsigned nClockSpeed = 0);

	/// \brief Constructor for SPI interface
	/// \param pSPIMaster Pointer to SPI master object
	/// \param nDCPin GPIO pin number for DC pin
	/// \param nResetPin GPIO pin number for Reset pin (active low)
	/// \param nChipSelect SPI chip select (0 or 1, default 0)
	/// \param nClockSpeed SPI clock frequency in Hz (default 8 MHz)
	/// \param nCPOL SPI clock polarity (0 or 1, default 0)
	/// \param nCPHA SPI clock phase (0 or 1, default 0)
	CSSD1309Display (CSPIMaster *pSPIMaster,
			 unsigned nDCPin, unsigned nResetPin,
			 unsigned nChipSelect = 0, unsigned nClockSpeed = 8000000,
			 unsigned nCPOL = 0, unsigned nCPHA = 0);

	~CSSD1309Display (void);

	/// \brief Set the global rotation of the display
	/// \param nDegrees Rotation in degrees (0 or 180, default 0)
	/// \note Must be set before calling Initialize().
	void SetRotation (unsigned nDegrees)	{ m_nRotation = nDegrees; }
	/// \return Rotation angle in degrees (0 or 180)
	unsigned GetRotation (void) const	{ return m_nRotation; }

	/// \return Operation successful?
	boolean Initialize (void);

	/// \return Display width in number of pixels
	unsigned GetWidth (void) const		{ return Width; }
	/// \return Display height in number of pixels
	unsigned GetHeight (void) const		{ return Height; }
	/// \return Number of bits per pixels
	unsigned GetDepth (void) const		{ return 1; }

	/// \brief Set display on
	void On (void);
	/// \brief Set display off
	void Off (void);

	/// \brief Clear entire display with color
	/// \param nColor Raw color value (I1, default Black)
	void Clear (TRawColor nColor = 0);

	/// \brief Set a single pixel to color
	/// \param nPosX X-position (0..width-1)
	/// \param nPosY Y-postion (0..height-1)
	/// \param nColor Raw color value (I1)
	void SetPixel (unsigned nPosX, unsigned nPosY, TRawColor nColor);

	/// \brief Set area (rectangle) on the display to the raw colors in pPixels
	/// \param rArea Coordinates of the area (zero-based)
	/// \param pPixels Pointer to array with raw color values (I1)
	/// \param pRoutine Routine to be called on completion
	/// \param pParam User parameter to be handed over to completion routine
	void SetArea (const TArea &rArea, const void *pPixels,
		      TAreaCompletionRoutine *pRoutine = nullptr,
		      void *pParam = nullptr);

private:
	void HardwareReset (void);

	boolean WriteCommand (u8 uchCommand);

	boolean WriteMemory (unsigned nColumnStart, unsigned nColumnEnd,
			     unsigned nPageStart, unsigned nPageEnd,
			     const void *pData, size_t ulDataSize);

private:
	boolean m_bSPI;

	// I2C specific
	CI2CMaster *m_pI2CMaster;
	u8 m_uchI2CAddress;
	unsigned m_nClockSpeed;

	// SPI specific
	CSPIMaster *m_pSPIMaster;
	unsigned m_nChipSelect;
	unsigned m_nSPIClockSpeed;
	unsigned m_nCPOL;
	unsigned m_nCPHA;
	CGPIOPin m_DCPin;

	// Common
	CGPIOPin m_ResetPin;
	unsigned m_nRotation;

	u8 m_Framebuffer[Height/8][Width];
};

#endif
