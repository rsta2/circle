//
/// \file 2dgraphics.h
//
// This file:
//	Copyright (C) 2021  Stephane Damo
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2024  R. Stange <rsta2@o2online.de>
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

#include <circle/display.h>
#include <circle/bcmframebuffer.h>
#include <circle/chargenerator.h>
#include <circle/types.h>

#define COLOR2D(red, green, blue)	DISPLAY_COLOR (red, green, blue)

typedef CDisplay::TColor T2DColor;

class C2DGraphics;

class C2DImage	/// A sprite image to be displayed on a C2DGraphics instance
{
public:
	/// \param p2DGraphics C2DGraphics instance to be used
	C2DImage (C2DGraphics *p2DGraphics);

	~C2DImage (void);

	/// \param nWidth Width of the image in number of pixels
	/// \param nHeight Height of the image in number of pixels
	/// \param pData Color information for the pixels in logical format
	void Set (unsigned nWidth, unsigned nHeight, const T2DColor *pData);

	/// \return Width of the image in number of pixels
	unsigned GetWidth (void) const;
	/// \return Height of the image in number of pixels
	unsigned GetHeight (void) const;

	/// \return Pointer to the color information of the pixels in hardware format
	const void *GetPixels (void) const;

private:
	CDisplay *m_pDisplay;

	unsigned m_nWidth;
	unsigned m_nHeight;

	size_t m_nDataSize;
	u8 *m_pData;
};

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
	/// \param pDisplay Pointer to display driver
	/// \note There is no VSync support with this constructor.
	C2DGraphics (CDisplay *pDisplay);

	/// \brief Uses CBcmFrameBuffer as display driver
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
	void ClearScreen (T2DColor Color);
	
	/// \brief Draws a filled rectangle
	/// \param nX Start X coordinate
	/// \param nY Start Y coordinate
	/// \param nWidth Rectangle width
	/// \param nHeight Rectangle height
	/// \param Color Rectangle color
	void DrawRect (unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight, T2DColor Color);
	
	/// \brief Draws an unfilled rectangle (inner outline)
	/// \param nX Start X coordinate
	/// \param nY Start Y coordinate
	/// \param nWidth Rectangle width
	/// \param nHeight Rectangle height
	/// \param Color Rectangle color
	void DrawRectOutline (unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight, T2DColor Color);
	
	/// \brief Draws a line
	/// \param nX1 Start position X coordinate
	/// \param nY1 Start position Y coordinate
	/// \param nX2 End position X coordinate
	/// \param nY2 End position Y coordinate
	/// \param Color Line color
	void DrawLine (unsigned nX1, unsigned nY1, unsigned nX2, unsigned nY2, T2DColor Color);
	
	/// \brief Draws a filled circle
	/// \param nX Circle X coordinate
	/// \param nY Circle Y coordinate
	/// \param nRadius Circle radius
	/// \param Color Circle color
	void DrawCircle (unsigned nX, unsigned nY, unsigned nRadius, T2DColor Color);
	
	/// \brief Draws an unfilled circle (inner outline)
	/// \param nX Circle X coordinate
	/// \param nY Circle Y coordinate
	/// \param nRadius Circle radius
	/// \param Color Circle color
	void DrawCircleOutline (unsigned nX, unsigned nY, unsigned nRadius, T2DColor Color);
	
	/// \brief Draws an image from a pixel buffer
	/// \param nX Image X coordinate
	/// \param nY Image Y coordinate
	/// \param nWidth Image width
	/// \param nHeight Image height
	/// \param PixelBuffer Pointer to the pixels
	void DrawImage (unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight, const void *PixelBuffer);
	
	/// \brief Draws an image from a pixel buffer with transparent color
	/// \param nX Image X coordinate
	/// \param nY Image Y coordinate
	/// \param nWidth Image width
	/// \param nHeight Image height
	/// \param PixelBuffer Pointer to the pixels
	/// \param TransparentColor Color to use for transparency
	void DrawImageTransparent (unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight, const void *PixelBuffer, T2DColor TransparentColor);
	
	/// \brief Draws an area of an image from a pixel buffer
	/// \param nX Image X coordinate
	/// \param nY Image Y coordinate
	/// \param nWidth Image width
	/// \param nHeight Image height
	/// \param nSourceX Source X coordinate in the pixel buffer
	/// \param nSourceY Source Y coordinate in the pixel buffer
	/// \param PixelBuffer Pointer to the pixels
	void DrawImageRect (unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight, unsigned nSourceX, unsigned nSourceY, const void *PixelBuffer);
	
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
	void DrawImageRectTransparent (unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight, unsigned nSourceX, unsigned nSourceY, unsigned nSourceWidth, unsigned nSourceHeight, const void *PixelBuffer, T2DColor TransparentColor);
	
	/// \brief Draws a single pixel. If you need to draw a lot of pixels, consider using GetBuffer() for better speed
	/// \param nX Pixel X coordinate
	/// \param nY Pixel Y coordinate
	/// \param Color Rectangle color
	void DrawPixel (unsigned nX, unsigned nY, T2DColor Color);

	/// \brief Draws a horizontal ISO8859-1 text string
	/// \param nX Text X coordinate
	/// \param nY Text Y coordinate
	/// \param Color Text color
	/// \param pText 0-terminated C-string
	/// \param Align Horizontal text alignment
	/// \param rFont Font to be used
	/// \param FontFlags Font flags
	/// \note Background is transparent
	void DrawText (unsigned nX, unsigned nY, T2DColor Color, const char *pText,
		       TTextAlign Align = AlignLeft, const TFont &rFont = DEFAULT_FONT,
		       CCharGenerator::TFontFlags FontFlags = CCharGenerator::FontFlagsNone);

	/// \brief Gets raw access to the drawing buffer
	/// \return Pointer to the buffer
	void *GetBuffer (void);

	/// \return Pointer to display, we are working on
	CDisplay *GetDisplay (void);
	
	/// \brief Once everything has been drawn, updates the display to show the contents on screen
	/// \brief If VSync is enabled, this method is blocking until the screen refresh signal is received (every 16ms for 60FPS refresh rate)
	void UpdateDisplay (void);

private:
	void SetPixel (unsigned nX, unsigned nY, CDisplay::TRawColor nColor)
	{
		switch (m_nDepth)
		{
		case 1: {
				u8 *pBuffer = &m_pBuffer8[(m_nWidth * nY + nX) / 8];
				u8 uchMask = 0x80 >> (nX & 7);
				if (nColor)
				{
					*pBuffer |= uchMask;
				}
				else
				{
					*pBuffer &= ~uchMask;
				}
			}
			break;

		case 8:		m_pBuffer8[m_nWidth * nY + nX] = nColor;	break;
		case 16:	m_pBuffer16[m_nWidth * nY + nX] = nColor;	break;
		case 32:	m_pBuffer32[m_nWidth * nY + nX] = nColor;	break;
		}
	}

	CDisplay::TRawColor GetPixel (const void **ppPixelBuffer, unsigned nOffsetX)
	{
		switch (m_nDepth)
		{
		case 1: {
				const u8 **pp = reinterpret_cast<const u8 **> (ppPixelBuffer);
				CDisplay::TRawColor nColor = !!(**pp & (0x80 >> (nOffsetX & 7)));
				if ((nOffsetX & 7) == 7)
				{
					(*pp)++;
				}
				return nColor;
			}

		case 8: {
				const u8 **pp = reinterpret_cast<const u8 **> (ppPixelBuffer);
				return *(*pp)++;
			}

		case 16: {
				const u16 **pp = reinterpret_cast<const u16 **> (ppPixelBuffer);
				return *(*pp)++;
			}

		case 32: {
				const u32 **pp = reinterpret_cast<const u32 **> (ppPixelBuffer);
				return *(*pp)++;
			}
		}

		return 0;
	}

private:
	unsigned m_nWidth;
	unsigned m_nHeight;
	unsigned m_nDepth;
	unsigned m_nDisplay;
	CDisplay *m_pDisplay;
	CBcmFrameBuffer	*m_pFrameBuffer;
	boolean m_bIsFrameBuffer;

	union
	{
		u8  *m_pBuffer8;
		u16 *m_pBuffer16;
		u32 *m_pBuffer32;
	};

	boolean m_bVSync;
	boolean m_bBufferSwapped;
};

#endif
