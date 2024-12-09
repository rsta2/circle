//
// ssd1306display.h
//
// Much of this driver is based on code from:
//	mt32-pi - A baremetal MIDI synthesizer for Raspberry Pi
//	Copyright (C) 2020-2022 Dale Whinham <daleyo@gmail.com>
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2018-2024  R. Stange <rsta2@o2online.de>
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
#include <display/ssd1306display.h>
#include <circle/util.h>
#include <assert.h>

// Prefix
#define DATA	0x40
#define CMD 	0x80

enum TSSD1306Command : u8
{
	SetMemoryAddressingMode    = 0x20,
	SetColumnAddress           = 0x21,
	SetPageAddress             = 0x22,
	SetStartLine               = 0x40,
	SetContrast                = 0x81,
	SetChargePump              = 0x8D,
	EntireDisplayOnResume      = 0xA4,
	SetNormalDisplay           = 0xA6,
	SetMultiplexRatio          = 0xA8,
	SetDisplayOff              = 0xAE,
	SetDisplayOn               = 0xAF,
	SetDisplayOffset           = 0xD3,
	SetDisplayClockDivideRatio = 0xD5,
	SetPrechargePeriod         = 0xD9,
	SetCOMPins                 = 0xDA,
	SetVCOMHDeselectLevel      = 0xDB
};

CSSD1306Display::CSSD1306Display (CI2CMaster *pI2CMaster,
				  unsigned nWidth, unsigned nHeight,
				  u8 uchI2CAddress, unsigned nClockSpeed)
:	CDisplay (I1),
	m_pI2CMaster (pI2CMaster),
	m_nWidth (nWidth),
	m_nHeight (nHeight),
	m_uchI2CAddress (uchI2CAddress),
	m_nClockSpeed (nClockSpeed),
	m_nRotation (0)
{
}

CSSD1306Display::~CSSD1306Display (void)
{
	Off ();
}

boolean CSSD1306Display::Initialize (void)
{
	if (   (   m_nHeight != 32
		&& m_nHeight != 64)
	    || m_nWidth != 128)
	{
		return FALSE;
	}

	const u8 nMultiplexRatio  = m_nHeight - 1;
	const u8 nCOMPins	  = m_nHeight == 32 ? 0x02 : 0x12;
	const u8 nSegRemap	  = m_nRotation == 180 ? 0xA0 : 0xA1;
	const u8 nCOMScanDir	  = m_nRotation == 180 ? 0xC0 : 0xC8;

	const u8 InitSequence[] =
	{
		SetDisplayOff,
		SetDisplayClockDivideRatio,	0x80,		// Default value
		SetMultiplexRatio,		nMultiplexRatio,// Screen height - 1
		SetDisplayOffset,		0x00,		// None
		SetStartLine | 0x00,				// Set start line
		SetChargePump,			0x14,		// Enable charge pump
		SetMemoryAddressingMode,	0x00,		// 00 = horizontal
		nSegRemap,
		nCOMScanDir,					// COM output scan direction
		SetCOMPins,			nCOMPins,	// Alternate COM config and disable COM left/right
		SetContrast,			0x7F,		// 00-FF, default to half
		SetPrechargePeriod,		0x22,		// Default value
		SetVCOMHDeselectLevel,		0x20,		// Default value
		EntireDisplayOnResume,				// Resume to RAM content display
		SetNormalDisplay,
		SetDisplayOn
	};

	for (u8 nCommand : InitSequence)
	{
		if (!WriteCommand (nCommand))
		{
			return FALSE;
		}
	}

	Clear ();

	return TRUE;
}

void CSSD1306Display::On (void)
{
	WriteCommand (SetDisplayOn);
}

void CSSD1306Display::Off (void)
{
	WriteCommand (SetDisplayOff);
}

void CSSD1306Display::Clear (TRawColor nColor)
{
	size_t ulSize = m_nWidth * m_nHeight/8;

	memset (m_Framebuffer, nColor ? 0xFF : 0x00, ulSize);

	WriteMemory (0, m_nWidth-1, 0, m_nHeight/8-1, m_Framebuffer, ulSize);
}

void CSSD1306Display::SetPixel (unsigned nPosX, unsigned nPosY, TRawColor nColor)
{
	if (   nPosX >= m_nWidth
	    || nPosY >= m_nHeight)
	{
		return;
	}

	u8 uchPage = nPosY / 8;
	u8 uchMask = 1 << (nPosY & 7);

	u8 *pBuffer = &m_Framebuffer[uchPage][nPosX];

	if (nColor)
	{
		*pBuffer |= uchMask;
	}
	else
	{
		*pBuffer &= ~uchMask;
	}

	WriteMemory (nPosX, nPosX, uchPage, uchPage, pBuffer, sizeof *pBuffer);
}

void CSSD1306Display::SetArea (const TArea &rArea, const void *pPixels,
			       TAreaCompletionRoutine *pRoutine, void *pParam)
{
	assert (rArea.x1 <= rArea.x2);
	assert (rArea.y1 <= rArea.y2);
	unsigned nWidth = rArea.x2 - rArea.x1 + 1;
	unsigned nBytesWidth = (nWidth + 7) / 8;

	// First transfer the pixel data into the framebuffer
	assert (pPixels);
	const u8 *pFrom = (const u8 *) pPixels;
	for (unsigned y = rArea.y1; y <= rArea.y2; y++)
	{
		unsigned y0 = y - rArea.y1;

		for (unsigned x = rArea.x1; x <= rArea.x2; x++)
		{
			unsigned x0 = x - rArea.x1;

			if (pFrom[x0/8 + y0 * nBytesWidth] & (0x80 >> (x0 & 7)))
			{
				m_Framebuffer[y/8][x] |= 1 << (y & 7);
			}
			else
			{
				m_Framebuffer[y/8][x] &= ~(1 << (y & 7));
			}
		}
	}

	// Now transfer the updated area to the display
	unsigned y1floor = rArea.y1 / 8 * 8;
	unsigned y2ceil = (rArea.y2 + 7) / 8 * 8 - 1;
	unsigned nHeight = y2ceil - y1floor + 1;

	u8 Buffer[nWidth * nHeight/8];
	for (unsigned y = y1floor; y <= y2ceil; y += 8)
	{
		unsigned y0 = y - y1floor;

		for (unsigned x = rArea.x1; x <= rArea.x2; x++)
		{
			unsigned x0 = x - rArea.x1;

			Buffer[x0 + y0/8 * nWidth] = m_Framebuffer[y/8][x];
		}
	}

	WriteMemory (rArea.x1, rArea.x2, rArea.y1/8, rArea.y2/8, Buffer, sizeof Buffer);

	if (pRoutine)
	{
		(*pRoutine) (pParam);
	}
}

boolean CSSD1306Display::WriteCommand (u8 uchCommand)
{
	assert (m_pI2CMaster);

	if (m_nClockSpeed)
	{
		m_pI2CMaster->SetClock (m_nClockSpeed);
	}

	u8 Cmd[] = {CMD, uchCommand};
	return m_pI2CMaster->Write (m_uchI2CAddress, Cmd, sizeof Cmd) == (int) sizeof Cmd;
}

boolean CSSD1306Display::WriteMemory (unsigned nColumnStart, unsigned nColumnEnd,
				      unsigned nPageStart, unsigned nPageEnd,
				      const void *pData, size_t ulDataSize)
{
	assert (m_pI2CMaster);

	if (m_nClockSpeed)
	{
		m_pI2CMaster->SetClock (m_nClockSpeed);
	}

	u8 Cmds[] = {CMD, SetColumnAddress, CMD, (u8) nColumnStart, CMD, (u8) nColumnEnd,
		     CMD, SetPageAddress,   CMD, (u8) nPageStart,   CMD, (u8) nPageEnd};
	if (m_pI2CMaster->Write (m_uchI2CAddress, Cmds, sizeof Cmds) != (int) sizeof Cmds)
	{
		return FALSE;
	}

	assert (pData);
	u8 Buffer[1 + ulDataSize];
	Buffer[0] = DATA;
	memcpy (Buffer + 1, pData, ulDataSize);

	return m_pI2CMaster->Write (m_uchI2CAddress, Buffer, sizeof Buffer) == (int) sizeof Buffer;
}
