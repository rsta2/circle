//
// windowdisplay.h
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
#ifndef _circle_windowdisplay_h
#define _circle_windowdisplay_h

#include <circle/display.h>

class CWindowDisplay : public CDisplay	/// Non-overlapping window on a display
{
public:
	/// \param pDisplay The display this window is displayed on
	/// \param rArea Area on pDisplay, which is covered by this window
	CWindowDisplay (CDisplay *pDisplay, const TArea &rArea);

	~CWindowDisplay (void);

	/// \return Number of horizontal pixels
	unsigned GetWidth (void) const override;
	/// \return Number of vertical pixels
	unsigned GetHeight (void) const override;
	/// \return Number of bits per pixel
	unsigned GetDepth (void) const override;

	/// \brief Set one pixel to physical color
	/// \param nPosX X-position of pixel (0-based)
	/// \param nPosY Y-position of pixel (0-based)
	/// \param nColor Raw color value (must match the color model)
	void SetPixel (unsigned nPosX, unsigned nPosY, TRawColor nColor) override;

	/// \brief Set area (rectangle) on the display to the raw colors in pPixels
	/// \param rArea Coordinates of the area (0-based)
	/// \param pPixels Pointer to array with raw color values
	/// \param pRoutine Routine to be called on completion (or nullptr for synchronous call)
	/// \param pParam User parameter to be handed over to completion routine
	void SetArea (const TArea &rArea, const void *pPixels,
		      TAreaCompletionRoutine *pRoutine = nullptr,
		      void *pParam = nullptr) override;

	/// \return Parent display
	CDisplay *GetParent (void) const override;
	/// \return X-offset in pixels of this window display in the parent display
	unsigned GetOffsetX (void) const override;
	/// \return Y-offset in pixels of this window display in the parent display
	unsigned GetOffsetY (void) const override;

private:
	CDisplay *m_pDisplay;
	TArea m_Area;
};

#endif
