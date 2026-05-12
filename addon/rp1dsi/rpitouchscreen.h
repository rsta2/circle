//
// rpitouchscreen.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2026  R. Stange <rsta2@gmx.net>
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
#ifndef _rp1dsi_rpitouchscreen_h
#define _rp1dsi_rpitouchscreen_h

#include <circle/display.h>
#include <circle/input/touchscreen.h>
#include <circle/interrupt.h>
#include <circle/screen.h>
#include <circle/i2cmaster.h>
#include <circle/types.h>
#include "rpitouchscreen1.h"
#include "rpitouchscreen2.h"
#include "rp1dsihostcontroller.h"

class CRPiTouchScreen : public CDisplay
{
public:
	/// \param pInterrupt Pointer to interrupt system object
	/// \param nDepth Number of bits per pixel in frame buffer (16 or 32)
	/// \param nDisplay Number of DISP port to use (0 or 1)
	/// \param bEnableTouch TRUE to enable touch function of the display
	CRPiTouchScreen (CInterruptSystem *pInterrupt,
			 unsigned nDepth = DEPTH, unsigned nDisplay = 0,
			 boolean bEnableTouch = TRUE);

	~CRPiTouchScreen (void);

	/// \brief Set the global rotation of the display (v1 display only)
	/// \param nDegrees Rotation in degrees (0, 180, default 0)
	/// \note Must be set before calling Initialize().
	void SetRotation (unsigned nDegrees)	{ m_nRotation = nDegrees; }
	/// \return Rotation angle in degrees (0, 180)
	unsigned GetRotation (void) const	{ return m_nRotation; }

	/// \return Operation successful?
	boolean Initialize (void);

	/// \return Display width in number of pixels
	unsigned GetWidth (void) const		{ return m_nWidth; }
	/// \return Display height in number of pixels
	unsigned GetHeight (void) const		{ return m_nHeight; }
	/// \return Number of bits per pixels
	unsigned GetDepth (void) const		{ return m_nDepth; }

	/// \brief Set a single pixel to color
	/// \param nPosX X-position (0..Width-1)
	/// \param nPosY Y-postion (0..Height-1)
	/// \param nColor Raw color value (RGB565 or ARGB8888)
	void SetPixel (unsigned nPosX, unsigned nPosY, TRawColor nColor);

	/// \brief Set area (rectangle) on the display to the raw colors in pPixels
	/// \param rArea Coordinates of the area (zero-based)
	/// \param pPixels Pointer to array with raw color values (RGB565 or ARGB8888)
	/// \param pRoutine Routine to be called on completion
	/// \param pParam User parameter to be handed over to completion routine
	void SetArea (const TArea &rArea, const void *pPixels,
		      TAreaCompletionRoutine *pRoutine = nullptr,
		      void *pParam = nullptr);

	/// \brief Set screen backlight brightness
	/// \param nBrightness Brightness level (0..255)
	/// \return Operation successful?
	boolean SetBacklightBrightness (unsigned nBrightness);

	typedef CRP1DSIHostController::TVBlankHandler TVerticalSyncHandler;
	/// \param pHandler Handler to be called in vertical blank pause
	/// \param pParam User pointer to be handed over to the handler
	void RegisterVerticalSyncHandler (TVerticalSyncHandler *pHandler, void *pParam = nullptr);

private:
	u8 GetRegID (void);

private:
	static const u8 I2CAddress = 0x45;	// of regulator

private:
	CInterruptSystem *m_pInterrupt;
	unsigned m_nDepth;
	unsigned m_nDisplay;
	boolean m_bEnableTouch;

	unsigned m_nRotation;

	unsigned m_nWidth;
	unsigned m_nHeight;
	unsigned m_nPitch;
	u8 *m_pFrameBuffer;

	CI2CMaster m_I2C;

	CRPiTouchScreen1 *m_pTouchScreen1;
	CRPiTouchScreen2 *m_pTouchScreen2;
};

#endif
