//
// screen.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2025  R. Stange <rsta2@o2online.de>
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
#include <circle/devicenameservice.h>

static const char DevicePrefix[] = "tty";

#ifndef SCREEN_HEADLESS

CScreenDevice::CScreenDevice (unsigned nWidth, unsigned nHeight, const TFont &rFont,
			      CCharGenerator::TFontFlags FontFlags, unsigned nDisplay)
:	m_nInitWidth (nWidth),
	m_nInitHeight (nHeight),
	m_nDisplay (nDisplay),
	m_rFont (rFont),
	m_FontFlags (FontFlags),
	m_pFrameBuffer (nullptr),
	m_pTerminal (nullptr)
{
}

CScreenDevice::~CScreenDevice (void)
{
	CDeviceNameService::Get ()->RemoveDevice (DevicePrefix, m_nDisplay+1, FALSE);

	delete m_pTerminal;
	m_pTerminal = nullptr;

	delete m_pFrameBuffer;
	m_pFrameBuffer = nullptr;
}

boolean CScreenDevice::Initialize (void)
{
	m_pFrameBuffer = new CBcmFrameBuffer (m_nInitWidth, m_nInitHeight, DEPTH,
					      0, 0, m_nDisplay);
	if (!m_pFrameBuffer)
	{
		return FALSE;
	}

#if DEPTH == 8
	m_pFrameBuffer->SetPalette (BLACK_COLOR, BLACK_COLOR);
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

	if (!m_pFrameBuffer->Initialize ())
	{
		return FALSE;
	}

	if (m_pFrameBuffer->GetDepth () != DEPTH)
	{
		return FALSE;
	}

	m_pTerminal = new CTerminalDevice (m_pFrameBuffer, m_nDisplay + 100, m_rFont, m_FontFlags);
	if (!m_pTerminal)
	{
		return FALSE;
	}

	if (!m_pTerminal->Initialize ())
	{
		return FALSE;
	}

	if (!CDeviceNameService::Get ()->GetDevice (DevicePrefix, m_nDisplay+1, FALSE))
	{
		CDeviceNameService::Get ()->AddDevice (DevicePrefix, m_nDisplay+1, this, FALSE);
	}

	return TRUE;
}

boolean CScreenDevice::Resize (unsigned nWidth, unsigned nHeight)
{
	delete m_pTerminal;
	m_pTerminal = nullptr;

	delete m_pFrameBuffer;
	m_pFrameBuffer = nullptr;

	m_nInitWidth = nWidth;
	m_nInitHeight = nHeight;

	return Initialize ();
}

unsigned CScreenDevice::GetWidth (void) const
{
	return m_pTerminal->GetWidth ();
}

unsigned CScreenDevice::GetHeight (void) const
{
	return m_pTerminal->GetHeight ();
}

unsigned CScreenDevice::GetColumns (void) const
{
	return m_pTerminal->GetColumns ();
}

unsigned CScreenDevice::GetRows (void) const
{
	return m_pTerminal->GetRows ();
}

CBcmFrameBuffer *CScreenDevice::GetFrameBuffer (void)
{
	return m_pFrameBuffer;
}

int CScreenDevice::Write (const void *pBuffer, size_t nCount)
{
	return m_pTerminal->Write (pBuffer, nCount);
}

void CScreenDevice::SetPixel (unsigned nPosX, unsigned nPosY, TScreenColor Color)
{
	m_pTerminal->SetPixel (nPosX, nPosY, Color);
}

TScreenColor CScreenDevice::GetPixel (unsigned nPosX, unsigned nPosY)
{
	return m_pTerminal->GetRawPixel (nPosX, nPosY);
}

void CScreenDevice::Rotor (unsigned nIndex, unsigned nCount)
{
	static u8 Pos[4][2] =
	{     // x, y
		{0, 1},
		{1, 0},
		{2, 1},
		{1, 2}
	};

	// in top right corner
	nIndex %= Rotors;
	unsigned nPosX = 2 + m_pTerminal->GetWidth () - (nIndex + 1) * 8;
	unsigned nPosY = 2;

	// Remove previous pixel
	nCount &= 4-1;
	SetPixel (nPosX + Pos[nCount][0], nPosY + Pos[nCount][1], BLACK_COLOR);

	// Set next pixel
	nCount++;
	nCount &= 4-1;
	SetPixel (nPosX + Pos[nCount][0], nPosY + Pos[nCount][1], HIGH_COLOR);
}

void CScreenDevice::SetCursorBlock (boolean bCursorBlock)
{
	m_pTerminal->SetCursorBlock (bCursorBlock);
}

void CScreenDevice::Update (unsigned nMillis)
{
	m_pTerminal->Update (nMillis);
}

#else	// #ifndef SCREEN_HEADLESS

CScreenDevice::CScreenDevice (unsigned nWidth, unsigned nHeight, const TFont &rFont,
			      CCharGenerator::TFontFlags FontFlags, unsigned nDisplay)
:	m_nInitWidth (nWidth),
	m_nInitHeight (nHeight),
	m_nDisplay (nDisplay),
	m_CharGen (rFont, FontFlags)
{
}

CScreenDevice::~CScreenDevice (void)
{
	CDeviceNameService::Get ()->RemoveDevice (DevicePrefix, m_nDisplay+1, FALSE);
}

boolean CScreenDevice::Initialize (void)
{
	if (   m_nInitWidth > 0
	    && m_nInitHeight > 0)
	{
		m_nWidth = m_nInitWidth;
		m_nHeight = m_nInitHeight;
	}
	else
	{
		m_nWidth = 640;
		m_nHeight = 480;
	}

	m_nUsedHeight = m_nHeight / m_CharGen.GetCharHeight () * m_CharGen.GetCharHeight ();

	if (!CDeviceNameService::Get ()->GetDevice (DevicePrefix, m_nDisplay+1, FALSE))
	{
		CDeviceNameService::Get ()->AddDevice (DevicePrefix, m_nDisplay+1, this, FALSE);
	}

	return TRUE;
}

boolean CScreenDevice::Resize (unsigned nWidth, unsigned nHeight)
{
	m_nInitWidth = nWidth;
	m_nInitHeight = nHeight;

	return Initialize ();
}

unsigned CScreenDevice::GetWidth (void) const
{
	return m_nWidth;
}

unsigned CScreenDevice::GetHeight (void) const
{
	return m_nHeight;
}

unsigned CScreenDevice::GetColumns (void) const
{
	return m_nWidth / m_CharGen.GetCharWidth ();
}

unsigned CScreenDevice::GetRows (void) const
{
	return m_nUsedHeight / m_CharGen.GetCharHeight ();
}

int CScreenDevice::Write (const void *pBuffer, size_t nCount)
{
	return nCount;
}

void CScreenDevice::SetPixel (unsigned nPosX, unsigned nPosY, TScreenColor Color)
{
}

TScreenColor CScreenDevice::GetPixel (unsigned nPosX, unsigned nPosY)
{
	return BLACK_COLOR;
}

void CScreenDevice::Rotor (unsigned nIndex, unsigned nCount)
{
}

void CScreenDevice::SetCursorBlock (boolean bCursorBlock)
{
}

void CScreenDevice::Update (unsigned nMillis)
{
}

#endif
