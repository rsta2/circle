//
// 2dgraphics.h
//
// This file:
//	Copyright (C) 2021  Stephane Damo
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2023  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_2dgraphics_h
#define _circle_2dgraphics_h

#include <circle/screen.h>
#include <circle/chargenerator.h>


class C2DGraphics /// Software graphics library with VSync and hardware-accelerated double buffering
{
public:
	enum TTextAlign
	{
		AlignLeft,
		AlignRight,
		AlignCenter
	};

public:
	/// \param nWidth   Screen width in pixels (0 to detect)
	/// \param nHeight  Screen height in pixels (0 to detect)
	/// \param bVSync   TRUE to enable VSync and HW double buffering
	/// \param nDisplay Zero-based display number (for Raspberry Pi 4)
	C2DGraphics (unsigned nWidth, unsigned nHeight, boolean bVSync = TRUE, unsigned nDisplay = 0);

	~C2DGraphics (void);

	/// \return Operation successful?
	boolean Initialize (void);

	/// \param nWidth  New screen width in pixels
	/// \param nHeight New screen height in pixels
	/// \return Operation successful?
	/// \note When FALSE is returned, the width and/or height are not supported.\n
	///	  The object is in an uninitialized state then and must not be used,\n
	///	  but Resize() can be called again with other parameters.
	boolean Resize (unsigned nWidth, unsigned nHeight);

	/// \return Screen width in pixels
	unsigned GetWidth () const;

	/// \return Screen height in pixels
	unsigned GetHeight () const;

	/// \brief Clears the screen
	/// \param Color Color used to clear the screen
	void ClearScreen (TScreenColor Color);
	
	/// \brief Draws a filled rectangle
	/// \param nX Start X coordinate
	/// \param nY Start Y coordinate
	/// \param nWidth Rectangle width
	/// \param nHeight Rectangle height
	/// \param Color Rectangle color
	void DrawRect (unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight, TScreenColor Color);
	
	/// \brief Draws an unfilled rectangle (inner outline)
	/// \param nX Start X coordinate
	/// \param nY Start Y coordinate
	/// \param nWidth Rectangle width
	/// \param nHeight Rectangle height
	/// \param Color Rectangle color
	void DrawRectOutline (unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight, TScreenColor Color);
	
	/// \brief Draws a line
	/// \param nX1 Start position X coordinate
	/// \param nY1 Start position Y coordinate
	/// \param nX2 End position X coordinate
	/// \param nY2 End position Y coordinate
	/// \param Color Line color
	void DrawLine (unsigned nX1, unsigned nY1, unsigned nX2, unsigned nY2, TScreenColor Color);
	
	/// \brief Draws a filled circle
	/// \param nX Circle X coordinate
	/// \param nY Circle Y coordinate
	/// \param nRadius Circle radius
	/// \param Color Circle color
	void DrawCircle (unsigned nX, unsigned nY, unsigned nRadius, TScreenColor Color);
	
	/// \brief Draws an unfilled circle (inner outline)
	/// \param nX Circle X coordinate
	/// \param nY Circle Y coordinate
	/// \param nRadius Circle radius
	/// \param Color Circle color
	void DrawCircleOutline (unsigned nX, unsigned nY, unsigned nRadius, TScreenColor Color);
	
	/// \brief Draws an image from a pixel buffer
	/// \param nX Image X coordinate
	/// \param nY Image Y coordinate
	/// \param nWidth Image width
	/// \param nHeight Image height
	/// \param PixelBuffer Pointer to the pixels
	void DrawImage (unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight, TScreenColor *PixelBuffer);
	
	/// \brief Draws an image from a pixel buffer with transparent color
	/// \param nX Image X coordinate
	/// \param nY Image Y coordinate
	/// \param nWidth Image width
	/// \param nHeight Image height
	/// \param PixelBuffer Pointer to the pixels
	/// \param TransparentColor Color to use for transparency
	void DrawImageTransparent (unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight, TScreenColor *PixelBuffer, TScreenColor TransparentColor);
	
	/// \brief Draws an area of an image from a pixel buffer
	/// \param nX Image X coordinate
	/// \param nY Image Y coordinate
	/// \param nWidth Image width
	/// \param nHeight Image height
	/// \param nSourceX Source X coordinate in the pixel buffer
	/// \param nSourceY Source Y coordinate in the pixel buffer
	/// \param PixelBuffer Pointer to the pixels
	void DrawImageRect (unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight, unsigned nSourceX, unsigned nSourceY, TScreenColor *PixelBuffer);
	
	/// \brief Draws an area of an image from a pixel buffer with transparent color
	/// \param nX Image X coordinate
	/// \param nY Image Y coordinate
	/// \param nWidth Image width
	/// \param nHeight Image height
	/// \param nSourceX Source X coordinate in the pixel buffer
	/// \param nSourceY Source Y coordinate in the pixel buffer
	/// \param nSourceWidth Source image width
	/// \param nSourceHeight Source image height
	/// \param PixelBuffer Pointer to the pixels
	/// \param TransparentColor Color to use for transparency
	void DrawImageRectTransparent (unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight, unsigned nSourceX, unsigned nSourceY, unsigned nSourceWidth, unsigned nSourceHeight, TScreenColor *PixelBuffer, TScreenColor TransparentColor);
	
	/// \brief Draws a single pixel. If you need to draw a lot of pixels, consider using GetBuffer() for better speed
	/// \param nX Pixel X coordinate
	/// \param nY Pixel Y coordinate
	/// \param Color Rectangle color
	void DrawPixel (unsigned nX, unsigned nY, TScreenColor Color);

	/// \brief Draws a horizontal ISO8859-1 text string
	/// \param nX Text X coordinate
	/// \param nY Text Y coordinate
	/// \param Color Text color
	/// \param pText 0-terminated C-string
	/// \param Align Horizontal text alignment
	/// \note Uses the 8x16 system font
	/// \note Background is transparent
	void DrawText (unsigned nX, unsigned nY, TScreenColor Color, const char *pText, TTextAlign Align = AlignLeft);

	/// \brief Gets raw access to the drawing buffer
	/// \return Pointer to the buffer
	TScreenColor *GetBuffer ();
	
	/// \brief Once everything has been drawn, updates the display to show the contents on screen
	/// \brief If VSync is enabled, this method is blocking until the screen refresh signal is received (every 16ms for 60FPS refresh rate)
	void UpdateDisplay();

private:
	unsigned m_nWidth;
	unsigned m_nHeight;
	unsigned m_nDisplay;
	CBcmFrameBuffer	*m_pFrameBuffer;
	TScreenColor *m_baseBuffer;
	TScreenColor *m_Buffer;
	boolean m_bVSync;
	boolean m_bBufferSwapped;

	CCharGenerator m_Font;
};

#endif
