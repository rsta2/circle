//
// ssd1309display.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2025  R. Stange <rsta2@o2online.de>
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
#include <display/ssd1309display.h>
#include <circle/timer.h>
#include <circle/util.h>
#include <assert.h>

// I2C prefix bytes
#define I2C_DATA	0x40
#define I2C_CMD		0x80

// SSD1309 commands (register-compatible with SSD1306)
enum TSSD1309Command : u8
{
	SetMemoryAddressingMode    = 0x20,
	SetColumnAddress           = 0x21,
	SetPageAddress             = 0x22,
	SetStartLine               = 0x40,
	SetContrast                = 0x81,
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

CSSD1309Display::CSSD1309Display (CI2CMaster *pI2CMaster,
				  unsigned nResetPin,
				  u8 uchI2CAddress, unsigned nClockSpeed)
:	CDisplay (I1),
	m_bSPI (FALSE),
	m_pI2CMaster (pI2CMaster),
	m_uchI2CAddress (uchI2CAddress),
	m_nClockSpeed (nClockSpeed),
	m_pSPIMaster (nullptr),
	m_nChipSelect (0),
	m_nSPIClockSpeed (0),
	m_nCPOL (0),
	m_nCPHA (0),
	m_nRotation (0)
{
	assert (pI2CMaster != nullptr);

	m_ResetPin.AssignPin (nResetPin);
	m_ResetPin.SetMode (GPIOModeOutput, FALSE);
}

CSSD1309Display::CSSD1309Display (CSPIMaster *pSPIMaster,
				  unsigned nDCPin, unsigned nResetPin,
				  unsigned nChipSelect, unsigned nClockSpeed,
				  unsigned nCPOL, unsigned nCPHA)
:	CDisplay (I1),
	m_bSPI (TRUE),
	m_pI2CMaster (nullptr),
	m_uchI2CAddress (0),
	m_nClockSpeed (0),
	m_pSPIMaster (pSPIMaster),
	m_nChipSelect (nChipSelect),
	m_nSPIClockSpeed (nClockSpeed),
	m_nCPOL (nCPOL),
	m_nCPHA (nCPHA),
	m_DCPin (nDCPin, GPIOModeOutput),
	m_nRotation (0)
{
	assert (pSPIMaster != nullptr);

	m_ResetPin.AssignPin (nResetPin);
	m_ResetPin.SetMode (GPIOModeOutput, FALSE);
}

CSSD1309Display::~CSSD1309Display (void)
{
	Off ();
}

boolean CSSD1309Display::Initialize (void)
{
	// Hardware reset is mandatory for SSD1309
	HardwareReset ();

	const u8 nSegRemap   = m_nRotation == 180 ? 0xA0 : 0xA1;
	const u8 nCOMScanDir = m_nRotation == 180 ? 0xC0 : 0xC8;

	const u8 InitSequence[] =
	{
		SetDisplayOff,
		SetDisplayClockDivideRatio,	0x80,		// Default value
		SetMultiplexRatio,		0x3F,		// 64 MUX (height - 1)
		SetDisplayOffset,		0x00,		// None
		SetStartLine | 0x00,				// Set start line
		SetMemoryAddressingMode,	0x00,		// 00 = horizontal
		nSegRemap,
		nCOMScanDir,					// COM output scan direction
		SetCOMPins,			0x12,		// Alternate COM config
		SetContrast,			0x7F,		// 00-FF, default to half
		SetPrechargePeriod,		0xF1,		// SSD1309: phase 1=1, phase 2=15
		SetVCOMHDeselectLevel,		0x34,		// SSD1309: ~0.78*Vcc
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

void CSSD1309Display::On (void)
{
	WriteCommand (SetDisplayOn);
}

void CSSD1309Display::Off (void)
{
	WriteCommand (SetDisplayOff);
}

void CSSD1309Display::Clear (TRawColor nColor)
{
	size_t ulSize = Width * Height/8;

	memset (m_Framebuffer, nColor ? 0xFF : 0x00, ulSize);

	WriteMemory (0, Width-1, 0, Height/8-1, m_Framebuffer, ulSize);
}

void CSSD1309Display::SetPixel (unsigned nPosX, unsigned nPosY, TRawColor nColor)
{
	if (   nPosX >= Width
	    || nPosY >= Height)
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

void CSSD1309Display::SetArea (const TArea &rArea, const void *pPixels,
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

void CSSD1309Display::HardwareReset (void)
{
	m_ResetPin.Write (HIGH);
	CTimer::SimpleMsDelay (10);
	m_ResetPin.Write (LOW);
	CTimer::SimpleMsDelay (10);
	m_ResetPin.Write (HIGH);
	CTimer::SimpleMsDelay (100);
}

boolean CSSD1309Display::WriteCommand (u8 uchCommand)
{
	if (m_bSPI)
	{
		assert (m_pSPIMaster);

		m_DCPin.Write (LOW);

		m_pSPIMaster->SetClock (m_nSPIClockSpeed);
		m_pSPIMaster->SetMode (m_nCPOL, m_nCPHA);

		return m_pSPIMaster->Write (m_nChipSelect, &uchCommand,
					    sizeof uchCommand) == (int) sizeof uchCommand;
	}
	else
	{
		assert (m_pI2CMaster);

		if (m_nClockSpeed)
		{
			m_pI2CMaster->SetClock (m_nClockSpeed);
		}

		u8 Cmd[] = {I2C_CMD, uchCommand};
		return m_pI2CMaster->Write (m_uchI2CAddress, Cmd,
					    sizeof Cmd) == (int) sizeof Cmd;
	}
}

boolean CSSD1309Display::WriteMemory (unsigned nColumnStart, unsigned nColumnEnd,
				      unsigned nPageStart, unsigned nPageEnd,
				      const void *pData, size_t ulDataSize)
{
	if (m_bSPI)
	{
		assert (m_pSPIMaster);

		// Send column/page address commands
		m_DCPin.Write (LOW);

		m_pSPIMaster->SetClock (m_nSPIClockSpeed);
		m_pSPIMaster->SetMode (m_nCPOL, m_nCPHA);

		u8 Cmds[] = {SetColumnAddress, (u8) nColumnStart, (u8) nColumnEnd,
			     SetPageAddress,   (u8) nPageStart,   (u8) nPageEnd};
		if (m_pSPIMaster->Write (m_nChipSelect, Cmds,
					 sizeof Cmds) != (int) sizeof Cmds)
		{
			return FALSE;
		}

		// Send data
		m_DCPin.Write (HIGH);

		assert (pData);
		return m_pSPIMaster->Write (m_nChipSelect, pData,
					    ulDataSize) == (int) ulDataSize;
	}
	else
	{
		assert (m_pI2CMaster);

		if (m_nClockSpeed)
		{
			m_pI2CMaster->SetClock (m_nClockSpeed);
		}

		u8 Cmds[] = {I2C_CMD, SetColumnAddress,
			     I2C_CMD, (u8) nColumnStart, I2C_CMD, (u8) nColumnEnd,
			     I2C_CMD, SetPageAddress,
			     I2C_CMD, (u8) nPageStart,   I2C_CMD, (u8) nPageEnd};
		if (m_pI2CMaster->Write (m_uchI2CAddress, Cmds,
					 sizeof Cmds) != (int) sizeof Cmds)
		{
			return FALSE;
		}

		assert (pData);
		u8 Buffer[1 + ulDataSize];
		Buffer[0] = I2C_DATA;
		memcpy (Buffer + 1, pData, ulDataSize);

		return m_pI2CMaster->Write (m_uchI2CAddress, Buffer,
					    sizeof Buffer) == (int) sizeof Buffer;
	}
}
