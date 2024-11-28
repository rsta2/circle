//
/// \file display.h
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
#ifndef _circle_display_h
#define _circle_display_h

#include <circle/types.h>

/// Logical display color (RGB888)
#define DISPLAY_COLOR(red, green, blue)	 (CDisplay::TColor) (  ((blue)  & 0xFF)		\
							     | ((green) & 0xFF) << 8	\
							     | ((red)   & 0xFF) << 16)

/// \note This class provides a generic interface for dot-matrix display drivers
///	  and provides methods for color conversion from logical to physical colors
///	  and back.

class CDisplay		/// Base class for dot-matrix display drivers
{
public:
	enum TColor : u32	/// Predefined logical colors (RGB888)
	{
		Black		= DISPLAY_COLOR (0, 0, 0),
		Red		= DISPLAY_COLOR (170, 0, 0),
		Green		= DISPLAY_COLOR (0, 170, 0),
		Yellow		= DISPLAY_COLOR (170, 85, 0),
		Blue		= DISPLAY_COLOR (0, 0, 170),
		Magenta		= DISPLAY_COLOR (170, 0, 170),
		Cyan		= DISPLAY_COLOR (0, 170, 170),
		White		= DISPLAY_COLOR (170, 170, 170),

		BrightBlack	= DISPLAY_COLOR (85, 85, 85),
		BrightRed	= DISPLAY_COLOR (255, 85, 85),
		BrightGreen	= DISPLAY_COLOR (85, 255, 85),
		BrightYellow	= DISPLAY_COLOR (255, 255, 85),
		BrightBlue	= DISPLAY_COLOR (85, 85, 255),
		BrightMagenta	= DISPLAY_COLOR (255, 85, 255),
		BrightCyan	= DISPLAY_COLOR (85, 255, 255),
		BrightWhite	= DISPLAY_COLOR (255, 255, 255),
	};

	static const TColor NormalColor	= BrightWhite;
	static const TColor HighColor	= BrightRed;
	static const TColor HalfColor	= Blue;

	enum TColorModel	/// Physical color model
	{
		RGB565,		///< 0bRRRRRGGG'GGGBBBBB
		RGB565_BE,	///< 0bGGGBBBBB'RRRRRGGG (Big endian)
		ARGB8888,	///< 0bAAAAAAAA'RRRRRRRR'GGGGGGGG'BBBBBBBB
		I1,		///< Black-white
		I8,		///< Index into palette

		ColorModelUnknown
	};

	typedef u32 TRawColor;	///< Physical color (matching color model)

	struct TArea		/// Coordinates are 0-based
	{
		unsigned x1, x2, y1, y2;
	};

	typedef void TAreaCompletionRoutine (void *pParam);

public:
	/// \param ColorModel Color model to be used
	CDisplay (TColorModel ColorModel);

	virtual ~CDisplay (void) {}

	/// \return Color model to be used
	TColorModel GetColorModel (void) const;

	/// \param Color Predefined logical color
	/// \return Raw color value for the color model
	TRawColor GetColor (TColor Color) const;
	/// \param Color Raw color value for the color model
	/// \return Predefined logical color (Black if not predefined)
	TColor GetColor (TRawColor Color) const;

	/// \return Number of horizontal pixels
	virtual unsigned GetWidth (void) const = 0;
	/// \return Number of vertical pixels
	virtual unsigned GetHeight (void) const = 0;
	/// \return Number of bits per pixel
	virtual unsigned GetDepth (void) const = 0;

	/// \brief Set one pixel to physical color
	/// \param nPosX X-position of pixel (0-based)
	/// \param nPosY Y-position of pixel (0-based)
	/// \param nColor Raw color value (must match the color model)
	virtual void SetPixel (unsigned nPosX, unsigned nPosY, TRawColor nColor) = 0;

	/// \brief Set area (rectangle) on the display to the raw colors in pPixels
	/// \param rArea Coordinates of the area (0-based)
	/// \param pPixels Pointer to array with raw color values
	/// \param pRoutine Routine to be called on completion (or nullptr for synchronous call)
	/// \param pParam User parameter to be handed over to completion routine
	virtual void SetArea (const TArea &rArea, const void *pPixels,
			      TAreaCompletionRoutine *pRoutine = nullptr,
			      void *pParam = nullptr) = 0;

	/// \return Parent display (nullptr if none)
	virtual CDisplay *GetParent (void) const;
	/// \return X-offset in pixels of this window display in the parent display
	virtual unsigned GetOffsetX (void) const;
	/// \return Y-offset in pixels of this window display in the parent display
	virtual unsigned GetOffsetY (void) const;

private:
	TColorModel m_ColorModel;
};

#endif
