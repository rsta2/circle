//
// windowdisplay.cpp
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
#include <circle/windowdisplay.h>
#include <assert.h>

CWindowDisplay::CWindowDisplay (CDisplay *pDisplay, const TArea &rArea)
:	CDisplay (pDisplay->GetColorModel ()),
	m_pDisplay (pDisplay),
	m_Area (rArea)
{
	assert (m_pDisplay);
}

CWindowDisplay::~CWindowDisplay (void)
{
	m_pDisplay = nullptr;
}

unsigned CWindowDisplay::GetWidth (void) const
{
	return m_Area.x2 - m_Area.x1 + 1;
}

unsigned CWindowDisplay::GetHeight (void) const
{
	return m_Area.y2 - m_Area.y1 + 1;
}

unsigned CWindowDisplay::GetDepth (void) const
{
	assert (m_pDisplay);
	return m_pDisplay->GetDepth ();
}

void CWindowDisplay::SetPixel (unsigned nPosX, unsigned nPosY, TRawColor nColor)
{
	if (   nPosX < GetWidth ()
	    && nPosY < GetHeight ())
	{
		assert (m_pDisplay);
		m_pDisplay->SetPixel (m_Area.x1 + nPosX, m_Area.y1 + nPosY, nColor);
	}
}

void CWindowDisplay::SetArea (const TArea &rArea, const void *pPixels,
			      TAreaCompletionRoutine *pRoutine, void *pParam)
{
	TArea Area;
	Area.x1 = m_Area.x1 + rArea.x1;
	Area.x2 = m_Area.x1 + rArea.x2;
	Area.y1 = m_Area.y1 + rArea.y1;
	Area.y2 = m_Area.y1 + rArea.y2;

	assert (m_pDisplay);
	if (   Area.x2 < m_pDisplay->GetWidth ()
	    && Area.y2 < m_pDisplay->GetHeight ())
	{
		m_pDisplay->SetArea (Area, pPixels, pRoutine, pParam);
	}
}

CDisplay *CWindowDisplay::GetParent (void) const
{
	return m_pDisplay;
}

unsigned CWindowDisplay::GetOffsetX (void) const
{
	return m_Area.x1;
}

unsigned CWindowDisplay::GetOffsetY (void) const
{
	return m_Area.y1;
}
