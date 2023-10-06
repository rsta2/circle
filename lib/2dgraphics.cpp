//
// 2dgraphics.cpp
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
#include <circle/2dgraphics.h>
#include <circle/screen.h>
#include <circle/bcmpropertytags.h>
#include <circle/util.h>

C2DGraphics::C2DGraphics (unsigned nWidth, unsigned nHeight, boolean bVSync, unsigned nDisplay)
: 	m_nWidth(nWidth),
	m_nHeight(nHeight),
	m_nDisplay (nDisplay),
	m_pFrameBuffer(0),
	m_Buffer(0),
	m_bVSync(bVSync),
	m_bBufferSwapped(TRUE)
{

}

C2DGraphics::~C2DGraphics (void)
{
	if(m_pFrameBuffer)
	{
		delete m_pFrameBuffer;
	}
}

boolean C2DGraphics::Initialize (void)
{
	m_pFrameBuffer = new CBcmFrameBuffer (m_nWidth, m_nHeight, DEPTH, m_nWidth, 2*m_nHeight,
					      m_nDisplay, TRUE);
	
#if DEPTH == 8
	m_pFrameBuffer->SetPalette (RED_COLOR, RED_COLOR16);
	m_pFrameBuffer->SetPalette (GREEN_COLOR, GREEN_COLOR16);
	m_pFrameBuffer->SetPalette (YELLOW_COLOR, YELLOW_COLOR16);
	m_pFrameBuffer->SetPalette (BLUE_COLOR, BLUE_COLOR16);
	m_pFrameBuffer->SetPalette (MAGENTA_COLOR, MAGENTA_COLOR16);
	m_pFrameBuffer->SetPalette (CYAN_COLOR, CYAN_COLOR16);
	m_pFrameBuffer->SetPalette (WHITE_COLOR, WHITE_COLOR16);
	m_pFrameBuffer->SetPalette (BRIGHT_BLACK_COLOR, BRIGHT_BLACK_COLOR16);
	m_pFrameBuffer->SetPalette (BRIGHT_RED_COLOR, BRIGHT_RED_COLOR16);
	m_pFrameBuffer->SetPalette (BRIGHT_GREEN_COLOR, BRIGHT_GREEN_COLOR16);
	m_pFrameBuffer->SetPalette (BRIGHT_YELLOW_COLOR, BRIGHT_YELLOW_COLOR16);
	m_pFrameBuffer->SetPalette (BRIGHT_BLUE_COLOR, BRIGHT_BLUE_COLOR16);
	m_pFrameBuffer->SetPalette (BRIGHT_MAGENTA_COLOR, BRIGHT_MAGENTA_COLOR16);
	m_pFrameBuffer->SetPalette (BRIGHT_CYAN_COLOR, BRIGHT_CYAN_COLOR16);
	m_pFrameBuffer->SetPalette (BRIGHT_WHITE_COLOR, BRIGHT_WHITE_COLOR16);
#endif

	if (!m_pFrameBuffer || !m_pFrameBuffer->Initialize ())
	{
		return FALSE;
	}

	m_baseBuffer = (TScreenColor *) (uintptr) m_pFrameBuffer->GetBuffer();
	m_nWidth = m_pFrameBuffer->GetWidth();
	m_nHeight = m_pFrameBuffer->GetHeight();
	m_Buffer = m_baseBuffer + m_nWidth * m_nHeight;
	
	return TRUE;
}

boolean C2DGraphics::Resize (unsigned nWidth, unsigned nHeight)
{
	delete m_pFrameBuffer;
	m_pFrameBuffer = 0;

	m_nWidth = nWidth;
	m_nHeight = nHeight;

	m_Buffer = 0;
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

void C2DGraphics::ClearScreen(TScreenColor Color)
{
	DrawRect(0, 0, m_nWidth, m_nHeight, Color);
}

void C2DGraphics::DrawRect (unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight, TScreenColor Color)
{
	if(nX + nWidth > m_nWidth || nY + nHeight > m_nHeight)
	{
		return;
	}
	
	for(unsigned i = nY; i < nY + nHeight; i++)
	{
		for(unsigned j = nX; j < nX + nWidth; j++)
		{
			m_Buffer[i * m_nWidth + j] = Color;
		}
	}
}

void C2DGraphics::DrawRectOutline (unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight, TScreenColor Color)
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

void C2DGraphics::DrawLine (unsigned nX1, unsigned nY1, unsigned nX2, unsigned nY2, TScreenColor Color)
{
	if(nX1 >= m_nWidth || nY1 >= m_nHeight || nX2 >= m_nWidth || nY2 >= m_nHeight)
	{
		return;
	}
	
	int dx = nX2 - nX1;
	int dy = nY2 - nY1;
	int dxabs = (dx>0) ? dx : -dx;
	int dyabs = (dy>0) ? dy : -dy;
	int sgndx = (dx>0) ? 1 : -1;
	int sgndy = (dy>0) ? 1 : -1;
	int x = dyabs >> 1;
	int y = dxabs >> 1;

	m_Buffer[m_nWidth * nY1 + nX1] = Color;

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
			m_Buffer[m_nWidth * nY1 + nX1] = Color;
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
			m_Buffer[m_nWidth * nY1 + nX1] = Color;
		}
	}
}

void C2DGraphics::DrawCircle (unsigned nX, unsigned nY, unsigned nRadius, TScreenColor Color)
{
	if(nX + nRadius >= m_nWidth || nY + nRadius >= m_nHeight || nX - nRadius >= m_nWidth || nY - nRadius >= m_nHeight)
	{
		return;
	}
	
	int r2 = nRadius * nRadius;
	unsigned area = r2 << 2;
	unsigned rr = nRadius << 1;

	for (unsigned i = 0; i < area; i++)
	{
		int tx = (int) (i % rr) - nRadius;
		int ty = (int) (i / rr) - nRadius;

		if (tx * tx + ty * ty < r2)
		{
			m_Buffer[m_nWidth * (nY + ty) + nX + tx] = Color;
		}
	}
}

void C2DGraphics::DrawCircleOutline (unsigned nX, unsigned nY, unsigned nRadius, TScreenColor Color)
{
	if(nX + nRadius >= m_nWidth || nY + nRadius >= m_nHeight || nX - nRadius >= m_nWidth || nY - nRadius >= m_nHeight)
	{
		return;
	}

	m_Buffer[m_nWidth * (nY) + nRadius + nX] = Color;

	if (nRadius > 0)
	{
		m_Buffer[m_nWidth * (-nRadius + nY) + nX] = Color;
		m_Buffer[m_nWidth * (nY) - nRadius + nX] = Color;
		m_Buffer[m_nWidth * (nRadius + nY) + nX] = Color;
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

		m_Buffer[m_nWidth * (y + nY) + nRadius + nX] = Color;
		m_Buffer[m_nWidth * (y + nY) - nRadius + nX] = Color;
		m_Buffer[m_nWidth * (-y + nY) + nRadius + nX] = Color;
		m_Buffer[m_nWidth * (-y + nY) - nRadius + nX] = Color;

		if ((int) nRadius != y)
		{
			m_Buffer[m_nWidth * (nRadius + nY) + y + nX] = Color;
			m_Buffer[m_nWidth * (nRadius + nY) - y + nX] = Color;
			m_Buffer[m_nWidth * (-nRadius + nY) + y + nX] = Color;
			m_Buffer[m_nWidth * (-nRadius + nY) - y + nX] = Color;
		}
	}
}

void C2DGraphics::DrawImage (unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight, TScreenColor *PixelBuffer)
{
	DrawImageRect(nX, nY, nWidth, nHeight, 0, 0, PixelBuffer);
}

void C2DGraphics::DrawImageTransparent (unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight, TScreenColor *PixelBuffer, TScreenColor TransparentColor)
{
	DrawImageRectTransparent(nX, nY, nWidth, nHeight, 0, 0, nWidth, nHeight, PixelBuffer, TransparentColor);
}

void C2DGraphics::DrawImageRect (unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight, unsigned nSourceX, unsigned nSourceY, TScreenColor *PixelBuffer)
{
	if(nX + nWidth > m_nWidth || nY + nHeight > m_nHeight)
	{
		return;
	}
	
	for(unsigned i=0; i<nHeight; i++)
	{
		for(unsigned j=0; j<nWidth; j++)
		{
			m_Buffer[(nY + i) * m_nWidth + j + nX] = PixelBuffer[(nSourceY + i) * nWidth + j + nSourceX];
		}
	}
}

void C2DGraphics::DrawImageRectTransparent (unsigned nX, unsigned nY, unsigned nWidth, unsigned nHeight, unsigned nSourceX, unsigned nSourceY, unsigned nSourceWidth, unsigned nSourceHeight, TScreenColor *PixelBuffer, TScreenColor TransparentColor)
{
	if(nX + nWidth > m_nWidth || nY + nHeight > m_nHeight || nSourceX + nWidth > nSourceWidth || nSourceY + nHeight > nSourceHeight)
	{
		return;
	}
	
	for(unsigned i=0; i<nHeight; i++)
	{
		for(unsigned j=0; j<nWidth; j++)
		{
			TScreenColor sourcePixel = PixelBuffer[(nSourceY + i) * nSourceWidth + j + nSourceX];
			if(sourcePixel != TransparentColor)
			{
				m_Buffer[(nY + i) * m_nWidth + j + nX] = sourcePixel;
			}
		}
	}
}

void C2DGraphics::DrawPixel (unsigned nX, unsigned nY, TScreenColor Color)
{
	if(nX >= m_nWidth || nY >= m_nHeight)
	{
		return;
	}
	
	m_Buffer[m_nWidth * nY + nX] = Color;
}

void C2DGraphics::DrawText (unsigned nX, unsigned nY, TScreenColor Color,
			    const char *pText, TTextAlign Align)
{
	unsigned nWidth = strlen (pText) * m_Font.GetCharWidth ();
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
	    || nY + m_Font.GetUnderline () > m_nHeight)
	{
		return;
	}

	for (; *pText != '\0'; pText++, nX += m_Font.GetCharWidth ())
	{
		for (unsigned y = 0; y < m_Font.GetUnderline (); y++)
		{
			for (unsigned x = 0; x < m_Font.GetCharWidth (); x++)
			{
				if (m_Font.GetPixel (*pText, x, y))
				{
					m_Buffer[(nY + y) * m_nWidth + x + nX] = Color;
				}
			}
		}
	}
}

TScreenColor* C2DGraphics::GetBuffer ()
{
	return m_Buffer;
}

void C2DGraphics::UpdateDisplay()
{
	
	if(m_bVSync)
	{
		m_pFrameBuffer->SetVirtualOffset(0, m_bBufferSwapped ? m_nHeight : 0);
		m_pFrameBuffer->WaitForVerticalSync();
		m_bBufferSwapped = !m_bBufferSwapped;
		m_Buffer = m_baseBuffer + m_bBufferSwapped * m_nWidth * m_nHeight;
	}
	else
	{
		memcpy(m_baseBuffer, m_Buffer, m_nWidth * m_nHeight * sizeof(TScreenColor));
	}
	
}
