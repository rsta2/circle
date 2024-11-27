//
// 2dgraphics.cpp
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
#include <circle/2dgraphics.h>
#include <circle/screen.h>
#include <circle/util.h>
#include <assert.h>

#define PTR_ADD(type, ptr, bytes)	((type) ((uintptr) (ptr) + (bytes)))

//// C2DImage //////////////////////////////////////////////////////////////////

C2DImage::C2DImage (C2DGraphics *p2DGraphics)
:	m_pDisplay (p2DGraphics->GetDisplay ()),
	m_nWidth (0),
	m_nHeight (0),
	m_nDataSize (0),
	m_pData (nullptr)
{
}

C2DImage::~C2DImage (void)
{
	delete [] m_pData;
	m_pData = nullptr;
}

void C2DImage::Set (unsigned nWidth, unsigned nHeight, const T2DColor *pData)
{
	assert (m_pDisplay);

	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_nDataSize = (nWidth * m_pDisplay->GetDepth () + 7) / 8 * nHeight;
	assert (m_nDataSize);

	delete [] m_pData;
	m_pData = new u8[m_nDataSize];
	assert (m_pData);

	assert (pData);

	switch (m_pDisplay->GetDepth ())
	{
	case 1: {
			u8 *p = m_pData;
			for (unsigned y = 0; y < m_nHeight; y++)
			{
				for (unsigned x = 0; x < m_nWidth; x += 8)
				{
					u8 uchByte = 0;
					for (unsigned i = 0; i < 8; i++)
					{
						if (x + i >= m_nWidth)
						{
							break;
						}

						uchByte |=   m_pDisplay->GetColor (*pData++)
							   ? 0x80 >> i : 0;
					}

					*p++ = uchByte;
				}
			}
		} break;

	case 8: {
			u8 *p = m_pData;
			for (unsigned i = 0; i < m_nDataSize; i += sizeof (u8))
			{
				*p++ = static_cast<u8> (m_pDisplay->GetColor (*pData++));
			}
		} break;

	case 16: {
			u16 *p = reinterpret_cast<u16 *> (m_pData);
			for (unsigned i = 0; i < m_nDataSize; i += sizeof (u16))
			{
				*p++ = static_cast<u16> (m_pDisplay->GetColor (*pData++));
			}
		} break;

	case 32: {
			u32 *p = reinterpret_cast<u32 *> (m_pData);
			for (unsigned i = 0; i < m_nDataSize; i += sizeof (u32))
			{
				*p++ = static_cast<u32> (m_pDisplay->GetColor (*pData++));
			}
		} break;
	}
}

unsigned C2DImage::GetWidth (void) const
{
	return m_nWidth;
}

unsigned C2DImage::GetHeight (void) const
{
	return m_nHeight;
}

const void *C2DImage::GetPixels (void) const
{
	assert (m_pData);

	return m_pData;
}

//// C2DGraphics ///////////////////////////////////////////////////////////////

C2DGraphics::C2DGraphics (CDisplay *pDisplay)
:	m_nWidth(0),
	m_nHeight(0),
	m_nDepth(0),
	m_pDisplay(pDisplay),
	m_pFrameBuffer(0),
	m_bIsFrameBuffer(FALSE),
	m_pBuffer8(0),
	m_bVSync(FALSE)
{
}

C2DGraphics::C2DGraphics (unsigned nWidth, unsigned nHeight, boolean bVSync, unsigned nDisplay)
: 	m_nWidth(nWidth),
	m_nHeight(nHeight),
	m_nDepth(0),
	m_nDisplay (nDisplay),
	m_pDisplay(0),
	m_pFrameBuffer(0),
	m_bIsFrameBuffer(TRUE),
	m_pBuffer8(0),
	m_bVSync(bVSync),
	m_bBufferSwapped(TRUE)
{

}

C2DGraphics::~C2DGraphics (void)
{
	delete [] m_pBuffer8;

	if(m_pFrameBuffer)
	{
		delete m_pFrameBuffer;
	}
}

boolean C2DGraphics::Initialize (void)
{
	if (m_bIsFrameBuffer)
	{
#if RASPPI <= 4
		m_pFrameBuffer = new CBcmFrameBuffer (m_nWidth, m_nHeight, DEPTH,
						      m_nWidth, 2*m_nHeight, m_nDisplay, TRUE);
#else
		m_pFrameBuffer = new CBcmFrameBuffer (m_nWidth, m_nHeight, DEPTH,
						      0, 0, m_nDisplay, FALSE);
#endif
		m_pDisplay = m_pFrameBuffer;
	
#if DEPTH == 8
		m_pFrameBuffer->SetPalette (0, BLACK_COLOR);
		m_pFrameBuffer->SetPalette (1, RED_COLOR16);
		m_pFrameBuffer->SetPalette (2, GREEN_COLOR16);
		m_pFrameBuffer->SetPalette (3, YELLOW_COLOR16);
		m_pFrameBuffer->SetPalette (4, BLUE_COLOR16);
		m_pFrameBuffer->SetPalette (5, MAGENTA_COLOR16);
		m_pFrameBuffer->SetPalette (6, CYAN_COLOR16);
		m_pFrameBuffer->SetPalette (7, WHITE_COLOR16);
		m_pFrameBuffer->SetPalette (8, BRIGHT_BLACK_COLOR16);
		m_pFrameBuffer->SetPalette (9, BRIGHT_RED_COLOR16);
		m_pFrameBuffer->SetPalette (10, BRIGHT_GREEN_COLOR16);
		m_pFrameBuffer->SetPalette (11, BRIGHT_YELLOW_COLOR16);
		m_pFrameBuffer->SetPalette (12, BRIGHT_BLUE_COLOR16);
		m_pFrameBuffer->SetPalette (13, BRIGHT_MAGENTA_COLOR16);
		m_pFrameBuffer->SetPalette (14, BRIGHT_CYAN_COLOR16);
		m_pFrameBuffer->SetPalette (15, BRIGHT_WHITE_COLOR16);
#endif

		if (!m_pFrameBuffer || !m_pFrameBuffer->Initialize ())
		{
			return FALSE;
		}
	}

	m_nWidth = m_pDisplay->GetWidth();
	m_nHeight = m_pDisplay->GetHeight();
	m_nDepth = m_pDisplay->GetDepth();

	if (   m_nDepth == 1
	    && m_nWidth % 8 != 0)
	{
		return FALSE;
	}

	m_pBuffer8 = new u8[m_nWidth * m_nHeight * m_nDepth/8];
	if (!m_pBuffer8)
	{
		return FALSE;
	}

	return TRUE;
}

boolean C2DGraphics::Resize (unsigned nWidth, unsigned nHeight)
{
	assert (m_bIsFrameBuffer);	// does work with frame buffer only

	delete m_pFrameBuffer;
	m_pFrameBuffer = 0;

	m_nWidth = nWidth;
	m_nHeight = nHeight;

	delete [] m_pBuffer8;
	m_pBuffer8 = 0;
	m_bBufferSwapped = TRUE;

	return Initialize ();
}

unsigned C2DGraphics::GetWidth () const
{
	return m_nWidth;
}

unsigned C2DGraphics::GetHeight () const
{
	return m_nHeight;
}

void C2DGraphics::ClearScreen(T2DColor Color)
{
	DrawRect(0, 0, m_nWidth, m_nHeight, Color);
}

void C2DGraphics::DrawRect (unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight, T2DColor Color)
{
	if(nX + nWidth > m_nWidth || nY + nHeight > m_nHeight)
	{
		return;
	}

	CDisplay::TRawColor nColor = m_pDisplay->GetColor (Color);
	
	for(unsigned i = nY; i < nY + nHeight; i++)
	{
		for(unsigned j = nX; j < nX + nWidth; j++)
		{
			SetPixel (j, i, nColor);
		}
	}
}

void C2DGraphics::DrawRectOutline (unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight, T2DColor Color)
{
	if(nX + nWidth > m_nWidth || nY + nHeight > m_nHeight)
	{
		return;
	}
	
	DrawLine(nX, nY, nX + nWidth, nY, Color);
	DrawLine(nX + nWidth, nY, nX + nWidth, nY + nHeight, Color);
	DrawLine(nX, nY + nHeight, nX + nWidth, nY + nHeight, Color);
	DrawLine(nX, nY, nX, nY + nHeight, Color);
}

void C2DGraphics::DrawLine (unsigned nX1, unsigned nY1, unsigned nX2, unsigned nY2, T2DColor Color)
{
	if(nX1 >= m_nWidth || nY1 >= m_nHeight || nX2 >= m_nWidth || nY2 >= m_nHeight)
	{
		return;
	}
	
	CDisplay::TRawColor nColor = m_pDisplay->GetColor (Color);

	int dx = nX2 - nX1;
	int dy = nY2 - nY1;
	int dxabs = (dx>0) ? dx : -dx;
	int dyabs = (dy>0) ? dy : -dy;
	int sgndx = (dx>0) ? 1 : -1;
	int sgndy = (dy>0) ? 1 : -1;
	int x = dyabs >> 1;
	int y = dxabs >> 1;

	SetPixel (nX1, nY1, nColor);

	if(dxabs >= dyabs)
	{
		for(int n=0; n<dxabs; n++)
		{
			y += dyabs;
			if(y >= dxabs)
			{
				y -= dxabs;
				nY1 += sgndy;
			}
			nX1 += sgndx;
			SetPixel (nX1, nY1, nColor);
		}
	}
	else
	{
		for(int n=0; n<dyabs; n++)
		{
			x += dxabs;
			if(x >= dyabs)
			{
				x -= dyabs;
				nX1 += sgndx;
			}
			nY1 += sgndy;
			SetPixel (nX1, nY1, nColor);
		}
	}
}

void C2DGraphics::DrawCircle (unsigned nX, unsigned nY, unsigned nRadius, T2DColor Color)
{
	if(nX + nRadius >= m_nWidth || nY + nRadius >= m_nHeight || nX - nRadius >= m_nWidth || nY - nRadius >= m_nHeight)
	{
		return;
	}
	
	CDisplay::TRawColor nColor = m_pDisplay->GetColor (Color);

	int r2 = nRadius * nRadius;
	unsigned area = r2 << 2;
	unsigned rr = nRadius << 1;

	for (unsigned i = 0; i < area; i++)
	{
		int tx = (int) (i % rr) - nRadius;
		int ty = (int) (i / rr) - nRadius;

		if (tx * tx + ty * ty < r2)
		{
			SetPixel (nX + tx, nY + ty, nColor);
		}
	}
}

void C2DGraphics::DrawCircleOutline (unsigned nX, unsigned nY, unsigned nRadius, T2DColor Color)
{
	if(nX + nRadius >= m_nWidth || nY + nRadius >= m_nHeight || nX - nRadius >= m_nWidth || nY - nRadius >= m_nHeight)
	{
		return;
	}

	CDisplay::TRawColor nColor = m_pDisplay->GetColor (Color);

	SetPixel (nRadius + nX, nY, nColor);

	if (nRadius > 0)
	{
		SetPixel (nX, -nRadius + nY, nColor);
		SetPixel (nRadius + nX, nY, nColor);
		SetPixel (nX, nRadius + nY, nColor);
	}
      
	int p = 1 - nRadius;
	int y = 0;
	
	while ((int) nRadius > y)
	{
		y++;

		if (p <= 0)
		{
			p = p + 2 * y + 1;
		}
		else
		{
			nRadius--;
			p = p + 2 * y - 2 * nRadius + 1;
		}

		if ((int) nRadius < y)
		{
			break;
		}

		SetPixel (nRadius + nX, y + nY, nColor);
		SetPixel (-nRadius + nX, y + nY, nColor);
		SetPixel (nRadius + nX, -y + nY, nColor);
		SetPixel (- nRadius + nX, -y + nY, nColor);

		if ((int) nRadius != y)
		{
			SetPixel (y + nX, nRadius + nY, nColor);
			SetPixel (-y + nX, nRadius + nY, nColor);
			SetPixel (y + nX, -nRadius + nY, nColor);
			SetPixel (-y + nX, -nRadius + nY, nColor);
		}
	}
}

void C2DGraphics::DrawImage (unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight, const void *PixelBuffer)
{
	DrawImageRect(nX, nY, nWidth, nHeight, 0, 0, PixelBuffer);
}

void C2DGraphics::DrawImageTransparent (unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight, const void *PixelBuffer, T2DColor TransparentColor)
{
	DrawImageRectTransparent(nX, nY, nWidth, nHeight, 0, 0, nWidth, nHeight, PixelBuffer, TransparentColor);
}

void C2DGraphics::DrawImageRect (unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight, unsigned nSourceX, unsigned nSourceY, const void *PixelBuffer)
{
	if(nX + nWidth > m_nWidth || nY + nHeight > m_nHeight)
	{
		return;
	}
	
	PixelBuffer = PTR_ADD (const void *, PixelBuffer,
			       (nSourceY * nWidth + nSourceX) * m_nDepth/8);

	for(unsigned i=0; i<nHeight; i++)
	{
		for(unsigned j=0; j<nWidth; j++)
		{
			SetPixel (nX + j, nY + i, GetPixel (&PixelBuffer, j));
		}
	}
}

void C2DGraphics::DrawImageRectTransparent (unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight, unsigned nSourceX, unsigned nSourceY, unsigned nSourceWidth, unsigned nSourceHeight, const void *PixelBuffer, T2DColor TransparentColor)
{
	if(nX + nWidth > m_nWidth || nY + nHeight > m_nHeight || nSourceX + nWidth > nSourceWidth || nSourceY + nHeight > nSourceHeight)
	{
		return;
	}
	
	for(unsigned i=0; i<nHeight; i++)
	{
		const void *pPixels =
			PTR_ADD (const void *, PixelBuffer,
				 ((nSourceY + i) * nSourceWidth + nSourceX) * m_nDepth/8);

		for(unsigned j=0; j<nWidth; j++)
		{
			CDisplay::TRawColor sourcePixel = GetPixel (&pPixels, j);
			if(sourcePixel != TransparentColor)
			{
				SetPixel (nX + j, nY + i, sourcePixel);
			}
		}
	}
}

void C2DGraphics::DrawPixel (unsigned nX, unsigned nY, T2DColor Color)
{
	if(nX >= m_nWidth || nY >= m_nHeight)
	{
		return;
	}

	CDisplay::TRawColor nColor = m_pDisplay->GetColor (Color);

	SetPixel (nX, nY, nColor);
}

void C2DGraphics::DrawText (unsigned nX, unsigned nY, T2DColor Color, const char *pText,
			    TTextAlign Align, const TFont &rFont,
			    CCharGenerator::TFontFlags FontFlags)
{
	CCharGenerator Font (rFont, FontFlags);

	unsigned nWidth = strlen (pText) * Font.GetCharWidth ();
	if (Align == AlignRight)
	{
		nX -= nWidth;
	}
	else if (Align == AlignCenter)
	{
		nX -= nWidth / 2;
	}

	if (   nX > m_nWidth
	    || nX + nWidth > m_nWidth
	    || nY + Font.GetUnderline () > m_nHeight)
	{
		return;
	}

	CDisplay::TRawColor nColor = m_pDisplay->GetColor (Color);

	for (; *pText != '\0'; pText++, nX += Font.GetCharWidth ())
	{
		for (unsigned y = 0; y < Font.GetUnderline (); y++)
		{
			CCharGenerator::TPixelLine Line = Font.GetPixelLine (*pText, y);

			for (unsigned x = 0; x < Font.GetCharWidth (); x++)
			{
				if (Font.GetPixel (x, Line))
				{
					SetPixel (nX + x, nY + y, nColor);
				}
			}
		}
	}
}

void *C2DGraphics::GetBuffer (void)
{
	return m_pBuffer8;
}

CDisplay *C2DGraphics::GetDisplay (void)
{
	return m_pDisplay;
}

void C2DGraphics::UpdateDisplay (void)
{
#if RASPPI <= 4
	if(m_bVSync)
	{
		unsigned nBaseHeight = m_bBufferSwapped ? m_nHeight : 0;
		CDisplay::TArea Area {0, m_nWidth-1, nBaseHeight, nBaseHeight + m_nHeight-1};

		m_pFrameBuffer->WaitForVerticalSync();
		m_pFrameBuffer->SetArea (Area, m_pBuffer8);
		m_pFrameBuffer->SetVirtualOffset(0, m_bBufferSwapped ? m_nHeight : 0);
		m_bBufferSwapped = !m_bBufferSwapped;
	}
	else
#endif
	{
		CDisplay::TArea Area {0, m_nWidth-1, 0, m_nHeight-1};

		if (!m_pDisplay)
		{
			m_pFrameBuffer->SetArea (Area, m_pBuffer8);
		}
		else
		{
			m_pDisplay->SetArea (Area, m_pBuffer8);
		}
	}
}
