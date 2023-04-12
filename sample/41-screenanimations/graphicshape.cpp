//
// kernel.cpp
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

#if DEPTH == 8
static const TScreenColor Black = NORMAL_COLOR;
static const TScreenColor Gray  = HALF_COLOR;
static const TScreenColor Red   = HIGH_COLOR;
#elif DEPTH == 16
static const TScreenColor Black = COLOR16 (0,0,0);
static const TScreenColor Gray  = COLOR16 (10,10,10);
static const TScreenColor Red   = COLOR16 (31,10,10);
#elif DEPTH == 32
static const TScreenColor Black = COLOR32 (0,0,0, 255);
static const TScreenColor Gray  = COLOR32 (170,170,170, 255);
static const TScreenColor Red   = COLOR32 (255,170,170, 255);
#else
	#error Screen DEPTH must be 8, 16 or 32!
#endif

static TScreenColor spriteData[64]=
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

CGraphicShape::CGraphicShape (unsigned nDisplayWidth, unsigned nDisplayHeight)
:	m_nDisplayWidth (nDisplayWidth),
	m_nDisplayHeight (nDisplayHeight)
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

#if DEPTH == 8
	m_Color = randomNumber() % 16;
#elif DEPTH == 16
	m_Color = randomNumber();
#elif DEPTH == 32
	m_Color = COLOR32 (randomNumber() % 256, randomNumber() % 256, randomNumber() % 256, 255);
#else
	#error Screen DEPTH must be 8, 16 or 32!
#endif
	
	m_nPosX = randomNumber()%m_nDisplayWidth;
	
	m_nPosY = randomNumber()%m_nDisplayHeight;
}

CGraphicShape::~CGraphicShape (void)
{
}

void CGraphicShape::Draw (C2DGraphics* renderer)
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
			renderer->DrawRect(m_nPosX, m_nPosY, m_nParam1, m_nParam2, m_Color);
			break;
			
		case GRAPHICSHAPE_OUTLINE:
			renderer->DrawRectOutline(m_nPosX, m_nPosY, m_nParam1, m_nParam2, m_Color);
			break;
			
		case GRAPHICSHAPE_LINE:
			renderer->DrawLine(m_nPosX, m_nPosY, m_nPosX+m_nParam1, m_nPosY+m_nParam2, m_Color);
			break;
			
		case GRAPHICSHAPE_CIRCLE:
			renderer->DrawCircle(m_nPosX, m_nPosY, m_nParam1, m_Color);
			break;
			
		case GRAPHICSHAPE_CIRCLEOUTLINE:
			renderer->DrawCircleOutline(m_nPosX, m_nPosY, m_nParam1, m_Color);
			break;
			
		case GRAPHICSHAPE_SPRITE_TRANSPARENTCOLOR:
			renderer->DrawImageTransparent(m_nPosX, m_nPosY, 8, 8, spriteData, Black);
			break;

		case GRAPHICSHAPE_TEXT:
			renderer->DrawText(m_nPosX, m_nPosY, m_Color, "Hello Circle!", C2DGraphics::AlignCenter);
			break;
	}
	
	
}
