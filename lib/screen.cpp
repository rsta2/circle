//
// screen.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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
#include <circle/screen.h>
#include <circle/util.h>

CScreenDevice::CScreenDevice (unsigned nWidth, unsigned nHeight)
:	m_nInitWidth (nWidth),
	m_nInitHeight (nHeight),
	m_pFrameBuffer (0),
	m_pBuffer (0)
{
}

CScreenDevice::~CScreenDevice (void)
{
	m_pBuffer = 0;
	
	delete m_pFrameBuffer;
	m_pFrameBuffer = 0;
}

boolean CScreenDevice::Initialize (void)
{
	m_pFrameBuffer = new CBcmFrameBuffer (m_nInitWidth, m_nInitHeight, DEPTH);
#if DEPTH == 8
	m_pFrameBuffer->SetPalette (NORMAL_COLOR, NORMAL_COLOR16);
	m_pFrameBuffer->SetPalette (HIGH_COLOR,   HIGH_COLOR16);
	m_pFrameBuffer->SetPalette (HALF_COLOR,   HALF_COLOR16);
#endif
	if (!m_pFrameBuffer->Initialize ())
	{
		return FALSE;
	}

	if (m_pFrameBuffer->GetDepth () != DEPTH)
	{
		return FALSE;
	}

	m_pBuffer = (TScreenColor *) m_pFrameBuffer->GetBuffer ();
	m_nSize   = m_pFrameBuffer->GetSize ();
	m_nWidth  = m_pFrameBuffer->GetWidth ();
	m_nHeight = m_pFrameBuffer->GetHeight ();

	// Makes things easier and is normally the case
	if (m_pFrameBuffer->GetPitch () != m_nWidth * sizeof (TScreenColor))
	{
		return FALSE;
	}

	// clear screen
	memset (m_pBuffer, 0, m_nSize);
	
	return TRUE;
}

unsigned CScreenDevice::GetWidth (void) const
{
	return m_nWidth;
}

unsigned CScreenDevice::GetHeight (void) const
{
	return m_nHeight;
}

void CScreenDevice::SetPixel (unsigned nPosX, unsigned nPosY, TScreenColor Color)
{
	if (   nPosX < m_nWidth
	    && nPosY < m_nHeight)
	{
		m_pBuffer[m_nWidth * nPosY + nPosX] = Color;
	}
}

TScreenColor CScreenDevice::GetPixel (unsigned nPosX, unsigned nPosY)
{
	if (   nPosX < m_nWidth
	    && nPosY < m_nHeight)
	{
		return m_pBuffer[m_nWidth * nPosY + nPosX];
	}
	
	return BLACK_COLOR;
}
