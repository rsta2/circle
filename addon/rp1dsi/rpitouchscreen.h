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
#include <circle/string.h>
#include <circle/types.h>
#include "drm_mipi_dsi.h"
#include "attinyregulator.h"
#include "rp1dsihostcontroller.h"
#include "tc358762dsibridge.h"

class CRPiTouchScreen : public CDisplay, mipi_dsi_device  /// EDT FT5x06 I2C Touchscreen Driver
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

	/// \brief Set the global rotation of the display
	/// \param nDegrees Rotation in degrees (0, 180, default 0)
	/// \note Must be set before calling Initialize().
	void SetRotation (unsigned nDegrees)	{ m_nRotation = nDegrees; }
	/// \return Rotation angle in degrees (0, 180)
	unsigned GetRotation (void) const	{ return m_nRotation; }

	/// \return Operation successful?
	boolean Initialize (void);

	/// \return Display width in number of pixels
	unsigned GetWidth (void) const;
	/// \return Display height in number of pixels
	unsigned GetHeight (void) const;
	/// \return Number of bits per pixels
	unsigned GetDepth (void) const;

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
	int edt_ft5x06_ts_identify (void);
	void edt_ft5x06_ts_isr (void);

	int regmap_read (u8 reg, int *pval);
	int regmap_bulk_read (u8 reg, void *buf, size_t buflen);

	static void UpdateStub (void *pParam);

private:
	static const u8 I2CAddress = 0x38;

	static const unsigned Width = 800;
	static const unsigned Height = 480;

	static const int MaxTouchPoints = 10;

	// for GENERIC_FT
	static const u8 tdata_cmd = 0;
	static const unsigned tdata_len = 3;
	static const unsigned tdata_offset = tdata_len;
	static const unsigned point_len = 6;

	enum edt_ver
	{
		EDT_M06,
		EDT_M09,
		EDT_M12,
		EV_FT,
		GENERIC_FT,
	};

private:
	CInterruptSystem *m_pInterrupt;
	unsigned m_nDepth;
	unsigned m_nDisplay;
	boolean m_bEnableTouch;

	unsigned m_nRotation;

	boolean m_bATTinyInitialized;

	CI2CMaster m_I2C;
	CATTinyRegulator m_ATTiny;
	CRP1DSIHostController m_DSI;
	CTC358762DSIBridge m_Bridge;

	u8 *m_pFrameBuffer;
	unsigned m_nPitch;

	CString m_model_name;
	edt_ver m_version;

	int m_init_td_status;
	unsigned m_nKnownIDs;
	unsigned m_nPosX[MaxTouchPoints];
	unsigned m_nPosY[MaxTouchPoints];

	CTouchScreenDevice *m_pInterface;

	static const drm_display_mode s_raspberrypi_7inch_mode;
};

#endif
