//
// kernel.cpp
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
#include "graphicshape.h"


static signed char sineTbl[256]=
{0,3,6,9,12,16,19,22,25,28,31,34,37,40,43,46,49,51,54,57,60,63,65,68,71,73,76,78
,81,83,85,88,90,92,94,96,98,100,102,104,106,107,109,111,112,113,115,116,117,118,
120,121,122,122,123,124,125,125,126,126,126,127,127,127,127,127,127,127,126,126,
126,125,125,124,123,122,122,121,120,118,117,116,115,113,112,111,109,107,106,104,
102,100,98,96,94,92,90,88,85,83,81,78,76,73,71,68,65,63,60,57,54,51,49,46,43,40,
37,34,31,28,25,22,19,16,12,9,6,3,0,-3,-6,-9,-12,-16,-19,-22,-25,-28,-31,-34,-37,
-40,-43,-46,-49,-51,-54,-57,-60,-63,-65,-68,-71,-73,-76,-78,-81,-83,-85,-88,-90,
-92,-94,-96,-98,-100,-102,-104,-106,-107,-109,-111,-112,-113,-115,-116,-117,-118
,-120,-121,-122,-122,-123,-124,-125,-125,-126,-126,-126,-127,-127,-127,-127,-127
,-127,-127,-126,-126,-126,-125,-125,-124,-123,-122,-122,-121,-120,-118,-117,-116
,-115,-113,-112,-111,-109,-107,-106,-104,-102,-100,-98,-96,-94,-92,-90,-88,-85,-
83,-81,-78,-76,-73,-71,-68,-65,-63,-60,-57,-54,-51,-49,-46,-43,-40,-37,-34,-31,-
28,-25,-22,-19,-16,-12,-9,-6,-3};

static unsigned g_seed;
static unsigned shapeType;

static const T2DColor Black = CDisplay::Black;
static const T2DColor Gray = CDisplay::White;
static const T2DColor Red = CDisplay::BrightRed;

static T2DColor spriteData[64]=
{
	Red,Gray,Gray,Gray,Gray,Gray,Gray,Red,
	Gray,Red,Gray,Gray,Gray,Gray,Red,Gray,
	Gray,Gray,Red,Gray,Gray,Red,Gray,Gray,
	Gray,Gray,Gray,Red,Red,Gray,Gray,Gray,
	Gray,Gray,Gray,Red,Red,Gray,Gray,Gray,
	Gray,Gray,Red,Black,Black,Red,Gray,Gray,
	Gray,Red,Black,Black,Black,Black,Red,Gray,
	Red,Black,Black,Black,Black,Black,Black,Red
};

static unsigned randomNumber()
{
	g_seed = (214013*g_seed+2531011);
	return (g_seed>>16)&0x7fff;
}

CGraphicShape::CGraphicShape (C2DGraphics *p2DGraphics)
:	m_p2DGraphics (p2DGraphics),
	m_nDisplayWidth (p2DGraphics->GetWidth ()),
	m_nDisplayHeight (p2DGraphics->GetHeight ()),
	m_Sprite (p2DGraphics)
{
	m_nSpeed = 0.5f+(randomNumber() % 256)*0.1f;

	m_nAngle = (randomNumber() % 256);

	m_nType = (shapeType++)%NB_GRAPHICSHAPE;
	
	m_nParam1 = 2+(randomNumber() % 32);
	
	
	
	if(m_nType == GRAPHICSHAPE_CIRCLE || m_nType == GRAPHICSHAPE_CIRCLEOUTLINE)
	{
		m_nParam2 = m_nParam1;
	}
	else
	{
		m_nParam2 = 2+(randomNumber() % 32);
	}

	m_Color = COLOR2D (randomNumber() % 256, randomNumber() % 256, randomNumber() % 256);
	
	m_nPosX = randomNumber()%m_nDisplayWidth;
	
	m_nPosY = randomNumber()%m_nDisplayHeight;

	if (m_nType == GRAPHICSHAPE_SPRITE_TRANSPARENTCOLOR)
	{
		m_Sprite.Set (8, 8, spriteData);
	}
}

CGraphicShape::~CGraphicShape (void)
{
}

void CGraphicShape::Draw (void)
{
	m_nPosX += m_nSpeed * sineTbl[(m_nAngle+64)%256]*(1.f/128)*0.1f;
	m_nPosY += m_nSpeed * sineTbl[m_nAngle]*(1.f/128)*0.1f;
	
	if(m_nPosX+m_nParam1 >= m_nDisplayWidth)
	{
		m_nPosX=m_nParam1;
	}
	else if(m_nPosX < m_nParam1)
	{
		m_nPosX=m_nDisplayWidth-m_nParam1;
	}
	
	if(m_nPosY+m_nParam2 >= m_nDisplayHeight)
	{
		m_nPosY=m_nParam2;
	}
	else if(m_nPosY < m_nParam2)
	{
		m_nPosY=m_nDisplayHeight-m_nParam2;
	}
	
	
	
	switch(m_nType)
	{
		case GRAPHICSHAPE_RECT:
			m_p2DGraphics->DrawRect(m_nPosX, m_nPosY, m_nParam1, m_nParam2, m_Color);
			break;
			
		case GRAPHICSHAPE_OUTLINE:
			m_p2DGraphics->DrawRectOutline(m_nPosX, m_nPosY, m_nParam1, m_nParam2, m_Color);
			break;
			
		case GRAPHICSHAPE_LINE:
			m_p2DGraphics->DrawLine(m_nPosX, m_nPosY, m_nPosX+m_nParam1, m_nPosY+m_nParam2, m_Color);
			break;
			
		case GRAPHICSHAPE_CIRCLE:
			m_p2DGraphics->DrawCircle(m_nPosX, m_nPosY, m_nParam1, m_Color);
			break;
			
		case GRAPHICSHAPE_CIRCLEOUTLINE:
			m_p2DGraphics->DrawCircleOutline(m_nPosX, m_nPosY, m_nParam1, m_Color);
			break;
			
		case GRAPHICSHAPE_SPRITE_TRANSPARENTCOLOR:
			m_p2DGraphics->DrawImageTransparent(m_nPosX, m_nPosY,
						       m_Sprite.GetWidth (), m_Sprite.GetHeight (),
						       m_Sprite.GetPixels (), Black);
			break;

		case GRAPHICSHAPE_TEXT:
			m_p2DGraphics->DrawText(m_nPosX, m_nPosY, m_Color, "Hello Circle!", C2DGraphics::AlignCenter);
			break;
	}
	
	
}
