//
// mousebehaviour.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2023  R. Stange <rsta2@o2online.de>
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
#include <circle/input/mousebehaviour.h>
#include <circle/bcmpropertytags.h>
#include <circle/bcm2835.h>
#include <assert.h>

#define MOUSE_BUTTONS		5

#define ACCELERATION		18		// 1/10

#define CURSOR_WIDTH		16
#define CURSOR_HEIGHT		16
#define CURSOR_HOTSPOT_X	0
#define CURSOR_HOTSPOT_Y	0

static const u32 CursorSymbol[CURSOR_HEIGHT][CURSOR_WIDTH] =
{
#define B	0
#define G	0xFF7F7F7FU
#define W	0xFF2F2F2FU
	{G,G,B,B,B,B,B,B,B,B,B,B,B,B,B,B},
	{G,W,G,B,B,B,B,B,B,B,B,B,B,B,B,B},
	{G,W,W,G,B,B,B,B,B,B,B,B,B,B,B,B},
	{G,W,W,W,G,B,B,B,B,B,B,B,B,B,B,B},
	{G,W,W,W,W,G,B,B,B,B,B,B,B,B,B,B},
	{G,W,W,W,W,W,G,B,B,B,B,B,B,B,B,B},
	{G,W,W,W,W,W,W,G,B,B,B,B,B,B,B,B},
	{G,W,W,W,W,W,W,W,G,B,B,B,B,B,B,B},
	{G,W,W,W,W,W,W,W,W,G,B,B,B,B,B,B},
	{G,G,G,G,W,W,G,G,G,G,G,B,B,B,B,B},
	{B,B,B,G,W,W,W,G,B,B,B,B,B,B,B,B},
	{B,B,B,B,G,W,W,G,B,B,B,B,B,B,B,B},
	{B,B,B,B,G,W,W,W,G,B,B,B,B,B,B,B},
	{B,B,B,B,B,G,W,W,G,B,B,B,B,B,B,B},
	{B,B,B,B,B,G,W,W,W,G,B,B,B,B,B,B},
	{B,B,B,B,B,B,G,G,G,G,B,B,B,B,B,B}
};

CMouseBehaviour::CMouseBehaviour (void)
:	m_nScreenWidth (0),
	m_nScreenHeight (0),
	m_nPosX (0),
	m_nPosY (0),
	m_bHasMoved (FALSE),
	m_bCursorOn (FALSE),
	m_nButtons (0),
	m_pEventHandler (0)
{
}

CMouseBehaviour::~CMouseBehaviour (void)
{
	m_pEventHandler = 0;

	if (m_bCursorOn)
	{
		SetCursorState (0, 0, FALSE);
	}
}

boolean CMouseBehaviour::Setup (unsigned nScreenWidth, unsigned nScreenHeight)
{
	assert (m_nScreenWidth == 0);
	m_nScreenWidth = nScreenWidth;
	assert (m_nScreenWidth > 0);

	assert (m_nScreenHeight == 0);
	m_nScreenHeight = nScreenHeight;
	assert (m_nScreenHeight > 0);

	m_nPosX = (m_nScreenWidth+1) / 2;
	m_nPosY = (m_nScreenHeight+1) / 2;

	CBcmPropertyTags Tags;
	TPropertyTagSetCursorInfo TagSetCursorInfo;
	TagSetCursorInfo.nWidth = CURSOR_WIDTH;
	TagSetCursorInfo.nHeight = CURSOR_HEIGHT;
	TagSetCursorInfo.nUnused = 0;
	TagSetCursorInfo.nPixelPointer = BUS_ADDRESS ((uintptr) CursorSymbol);
	TagSetCursorInfo.nHotspotX = CURSOR_HOTSPOT_X;
	TagSetCursorInfo.nHotspotY = CURSOR_HOTSPOT_Y;
	if (!Tags.GetTag (PROPTAG_SET_CURSOR_INFO, &TagSetCursorInfo, sizeof TagSetCursorInfo, 6*4))
	{
		return FALSE;
	}

	if (TagSetCursorInfo.nResponse != CURSOR_RESPONSE_VALID)
	{
		return FALSE;
	}

	return TRUE;
}

void CMouseBehaviour::Release (void)
{
	if (   m_nScreenWidth == 0		// not setup?
	    || m_nScreenHeight == 0)
	{
		return;
	}

	if (m_bCursorOn)
	{
		SetCursorState (0, 0, FALSE);
	}

	m_nScreenWidth = 0;
	m_nScreenHeight = 0;
	m_nPosX = 0;
	m_nPosY = 0;
	m_bHasMoved = FALSE;
	m_bCursorOn = FALSE;
	m_nButtons = 0;
}

void CMouseBehaviour::RegisterEventHandler (TMouseEventHandler *pEventHandler)
{
	assert (m_pEventHandler == 0);
	m_pEventHandler = pEventHandler;
	assert (m_pEventHandler != 0);
}

boolean CMouseBehaviour::SetCursor (unsigned nPosX, unsigned nPosY)
{
	assert (m_nScreenWidth > 0);
	assert (m_nScreenHeight > 0);
	if (   nPosX >= m_nScreenWidth
	    || nPosY >= m_nScreenHeight)
	{
		return FALSE;
	}

	m_nPosX = nPosX;
	m_nPosY = nPosY;

	if (!m_bCursorOn)
	{
		return TRUE;
	}

	return SetCursorState (m_nPosX, m_nPosY, TRUE);
}

boolean CMouseBehaviour::ShowCursor (boolean bShow)
{
	boolean bResult = m_bCursorOn;
	m_bCursorOn = bShow;

	assert (m_nPosX < m_nScreenWidth);
	assert (m_nPosY < m_nScreenHeight);
	SetCursorState (m_nPosX, m_nPosY, m_bCursorOn);

	return bResult;
}

void CMouseBehaviour::UpdateCursor (void)
{
	if (   m_bCursorOn
	    && m_bHasMoved)
	{
		m_bHasMoved = FALSE;

		SetCursorState (m_nPosX, m_nPosY, TRUE);
	}
}

void CMouseBehaviour::MouseStatusChanged (unsigned nButtons, int nDisplacementX, int nDisplacementY, int nWheelMove)
{
	if (   m_nScreenWidth == 0		// not setup?
	    || m_nScreenHeight == 0)
	{
		return;
	}

	nDisplacementX = nDisplacementX * ACCELERATION / 10;
	nDisplacementY = nDisplacementY * ACCELERATION / 10;

	unsigned nPrevX = m_nPosX;
	unsigned nPrevY = m_nPosY;

	m_nPosX = (int) m_nPosX + nDisplacementX;
	if (m_nPosX >= m_nScreenWidth)
	{
		m_nPosX = (int) m_nPosX - nDisplacementX;
	}

	m_nPosY = (int) m_nPosY + nDisplacementY;
	if (m_nPosY >= m_nScreenHeight)
	{
		m_nPosY = (int) m_nPosY - nDisplacementY;
	}

	if (   m_nPosX != nPrevX
	    || m_nPosY != nPrevY)
	{
		m_bHasMoved = TRUE;

		if (m_pEventHandler != 0)
		{
			(*m_pEventHandler) (MouseEventMouseMove, nButtons, m_nPosX, m_nPosY, nWheelMove);
		}
	}

	if (   m_nButtons != nButtons
	    && m_pEventHandler != 0)
	{
		for (unsigned nButton = 0; nButton < MOUSE_BUTTONS; nButton++)
		{
			unsigned nMask = 1 << nButton;

			if (   !(m_nButtons & nMask)
			    &&  (nButtons & nMask))
			{
				(*m_pEventHandler) (MouseEventMouseDown, nMask, m_nPosX, m_nPosY, nWheelMove);
			}
			else if (   (m_nButtons & nMask)
				 && !(nButtons & nMask))
			{
				(*m_pEventHandler) (MouseEventMouseUp, nMask, m_nPosX, m_nPosY, nWheelMove);
			}
		}
	}

	if (nWheelMove != 0) {
		if (m_pEventHandler != 0)
		{
			(*m_pEventHandler) (MouseEventMouseWheel, nButtons, m_nPosX, m_nPosY, nWheelMove);
		}
	}

	m_nButtons = nButtons;
}

boolean CMouseBehaviour::SetCursorState (unsigned nPosX, unsigned nPosY, boolean bVisible)
{
	CBcmPropertyTags Tags;
	TPropertyTagSetCursorState TagSetCursorState;
	TagSetCursorState.nEnable = bVisible ? CURSOR_ENABLE_VISIBLE : CURSOR_ENABLE_INVISIBLE;
	TagSetCursorState.nPosX = nPosX;
	TagSetCursorState.nPosY = nPosY;
	TagSetCursorState.nFlags = CURSOR_FLAGS_FB_COORDS;
	if (!Tags.GetTag (PROPTAG_SET_CURSOR_STATE, &TagSetCursorState, sizeof TagSetCursorState, 4*4))
	{
		return FALSE;
	}

	if (TagSetCursorState.nResponse != CURSOR_RESPONSE_VALID)
	{
		return FALSE;
	}

	return TRUE;
}
