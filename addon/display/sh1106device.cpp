//
// sh1106device.cpp
//
// Much of this driver is based on code from:
//    mt32-pi - A baremetal MIDI synthesizer for Raspberry Pi
//    Copyright (C) 2020-2022 Dale Whinham <daleyo@gmail.com>
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2018-2022  R. Stange <rsta2@o2online.de>
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
#include "sh1106device.h"
#include <circle/timer.h>
#include <circle/types.h>
#include <circle/util.h>
#include <assert.h>
#include "font6x8.h"

namespace
{
	constexpr u8 kColumnOffset = 2;
}

enum TSH1106Command : u8
{
	SetColumnAddressLow        = 0x00,
	SetColumnAddressHigh       = 0x10,
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
	SetVCOMHDeselectLevel      = 0xDB,
	SetPageAddress             = 0xB0,
};

// Compile-time (constexpr) font/graphics conversion functions.
// The SH1106 stores pixel data in columns, but our source data is stored as rows.
// These templated functions generate column-wise versions of our graphics at compile-time.
namespace
{
	using CharData = u8[8];

	// Iterate through each row of the character data and collect bits for the nth column
	static constexpr u8 SingleColumn(const CharData& CharData, u8 nColumn)
	{
		u8 bit = 5 - nColumn;
		u8 column = 0;

		for (u8 i = 0; i < 8; ++i)
			column |= (CharData[i] >> bit & 1) << i;

		return column;
	}

	// Double the height of the character by duplicating column bits into a 16-bit value
	static constexpr u16 DoubleColumn(const CharData& CharData, u8 nColumn)
	{
		u8 singleColumn = SingleColumn(CharData, nColumn);
		u16 column = 0;

		for (u8 i = 0; i < 8; ++i)
		{
			bool bit = singleColumn >> i & 1;
			column |= bit << i * 2 | bit << (i * 2 + 1);
		}

		return column;
	}

	// Templated array-like structure with precomputed font data
	template<size_t N, class F>
	class Font
	{
	public:
		// Result type of conversion function should determine array type, but fixed here to u16
		using Column = u16;
		using ColumnData = Column[6];

		constexpr Font(const CharData(&CharData)[N], F Function) : mCharData{ {0} }
		{
			for (size_t i = 0; i < N; ++i)
				for (u8 j = 0; j < 6; ++j)
					mCharData[i][j] = Function(CharData[i], j);
		}

		const ColumnData& operator[](size_t nIndex) const { return mCharData[nIndex]; }

	private:
		ColumnData mCharData[N];
	};
}

// Single and double-height versions of the font
// constexpr auto FontSingle = Font<FONT_SIZE, decltype(SingleColumn)>(Font6x8, SingleColumn);
constexpr auto FontDouble = Font<FONT_SIZE, decltype(DoubleColumn)>(Font6x8, DoubleColumn);


CSH1106Device::CSH1106Device (unsigned nWidth, unsigned nHeight,
			      CI2CMaster *pI2CMaster, u8 nAddress,
			      unsigned nRotate, bool mirrored)
:	CCharDevice (SH1106_COLUMNS, SH1106_ROWS),
	m_nWidth (nWidth),
	m_nHeight (nHeight),
	m_pI2CMaster (pI2CMaster),
	m_nAddress (nAddress),
	m_nRotate ((nRotate == 90 || nRotate == 180 || nRotate == 270) ? nRotate : 0),
	m_bMirrored (mirrored),
	m_FrameBuffers{{0x40, {0}}, {0x40, {0}}},
	m_nCurrentFrameBuffer(0)
{
}

CSH1106Device::~CSH1106Device (void)
{
}

boolean CSH1106Device::Initialize (void)
{
	assert(m_pI2CMaster != nullptr);

	// Validate dimensions - only 128x32 and 128x64 supported for now
	if (!(m_nHeight == 32 || m_nHeight == 64) || m_nWidth != 128)
		return false;

	const u8 nMultiplexRatio  = m_nHeight - 1;
	const u8 nCOMPins         = m_nHeight == 32 ? 0x02 : 0x12;
	// https://www.buydisplay.com/download/ic/SSD1312_Datasheet.pdf Pg. 51 Section 2.1.19
	//            normal    inverted
	// normal     A1 C8       A0 C0
	// mirrored   A0 C8       A1 C0
	const u8 nSegRemap        = m_bMirrored ? 0xA0 : 0xA1;
	const u8 nCOMScanDir      = 0xC8;

	const u8 InitSequence[] =
	{
		SetDisplayOff,
		SetDisplayClockDivideRatio,	0x80,				// Default value
		SetMultiplexRatio,		nMultiplexRatio,		// Screen height - 1
		SetDisplayOffset,		0x00,				// None
		SetStartLine | 0x00,						// Set start line
		SetChargePump,			0x14,				// Enable charge pump
		nSegRemap,
		nCOMScanDir,							// COM output scan direction
		SetCOMPins,			nCOMPins,			// Alternate COM config and disable COM left/right
		SetContrast,			0x7F,				// 00-FF, default to half
		SetPrechargePeriod,		0x22,				// Default value
		SetVCOMHDeselectLevel,		0x20,				// Default value
		EntireDisplayOnResume,						// Resume to RAM content display
		SetNormalDisplay,
		SetDisplayOn,
	};

	for (u8 nCommand : InitSequence)
		WriteCommand(nCommand);

	return CCharDevice::Initialize();
}

void CSH1106Device::DevClearCursor (void)
{
	// Just clear the display
	Clear(TRUE);
}

void CSH1106Device::DevSetCursorMode (boolean bVisible)
{
}

void CSH1106Device::DevSetChar (unsigned nPosX, unsigned nPosY, char chChar)
{
	// Draw a non-inverted, non-double width character
	DrawChar (chChar, nPosX, nPosY, FALSE, FALSE);
}

void CSH1106Device::DevSetCursor (unsigned nCursorX, unsigned nCursorY)
{
}

void CSH1106Device::DevUpdateDisplay (void) {
	WriteFrameBuffer(true);
}

void CSH1106Device::WriteCommand(u8 nCommand) const
{
	const u8 Buffer[] = { 0x80, nCommand };
	m_pI2CMaster->Write(m_nAddress, Buffer, sizeof(Buffer));
}

void CSH1106Device::PutPixel(u8 *pFrameBuffer, unsigned nX, unsigned nY, bool bSet) const
{
	if (nX >= m_nWidth || nY >= m_nHeight)
		return;

	u8 &nByte = pFrameBuffer[(nY / 8) * m_nWidth + nX];
	const u8 nMask = 1 << (nY % 8);

	if (bSet)
		nByte |= nMask;
	else
		nByte &= ~nMask;
}

void CSH1106Device::PutRotatedPixel(u8 *pFrameBuffer, unsigned nX, unsigned nY, bool bSet) const
{
	switch (m_nRotate)
	{
	case 90:
		PutPixel(pFrameBuffer, nY / 2, (m_nWidth - 1 - nX) / 2, bSet);
		break;

	case 180:
		PutPixel(pFrameBuffer, m_nWidth - 1 - nX, m_nHeight - 1 - nY, bSet);
		break;

	case 270:
		PutPixel(pFrameBuffer, (m_nHeight - 1 - nY) / 2, nX / 2, bSet);
		break;

	case 0:
	default:
		PutPixel(pFrameBuffer, nX, nY, bSet);
		break;
	}
}

void CSH1106Device::PutColumn(u8 *pFrameBuffer, unsigned nX, unsigned nPageY, u8 nColumn) const
{
	for (unsigned nBit = 0; nBit < 8; ++nBit)
		PutRotatedPixel(pFrameBuffer, nX, nPageY * 8 + nBit, nColumn & (1 << nBit));
}

void CSH1106Device::WriteFrameBuffer(bool bForceFullUpdate) const
{
	// Reset start line
	WriteCommand(SetStartLine | 0x00);

	// Compare two framebuffers
	const size_t nFrameBufferSize = m_nWidth * m_nHeight / 8;
	const bool bNeedsUpdate = bForceFullUpdate || memcmp(m_FrameBuffers[0].FrameBuffer, m_FrameBuffers[1].FrameBuffer, nFrameBufferSize) != 0;

	if (!bNeedsUpdate)
		return;

	assert(m_nWidth <= 128);
	const unsigned nPages = m_nHeight / 8;
	const u8 *pFrameBuffer = m_FrameBuffers[m_nCurrentFrameBuffer].FrameBuffer;

	for (unsigned nPage = 0; nPage < nPages; ++nPage)
	{
		const u8 nColumnStart = kColumnOffset;
		const u8 Cmds[] =
		{
			0x80, static_cast<u8>(SetPageAddress | nPage),
			0x80, static_cast<u8>(SetColumnAddressLow | (nColumnStart & 0x0F)),
			0x80, static_cast<u8>(SetColumnAddressHigh | ((nColumnStart >> 4) & 0x0F)),
		};
		m_pI2CMaster->Write(m_nAddress, Cmds, sizeof(Cmds));

		u8 Buffer[1 + 128];
		Buffer[0] = 0x40;
		memcpy(Buffer + 1, pFrameBuffer + nPage * m_nWidth, m_nWidth);
		m_pI2CMaster->Write(m_nAddress, Buffer, 1 + m_nWidth);
	}
}

void CSH1106Device::SwapFrameBuffers()
{
	// Make other framebuffer current
	m_nCurrentFrameBuffer = (m_nCurrentFrameBuffer + 1) % 2;
}

void CSH1106Device::DrawChar(char chChar, u8 nCursorX, u8 nCursorY, bool bInverted, bool bDoubleWidth)
{
	const size_t nColumnOffset = nCursorX * (bDoubleWidth ? 12 : 6) + 4;
	u8* pFrameBuffer           = m_FrameBuffers[m_nCurrentFrameBuffer].FrameBuffer;

	// FIXME: Won't be needed when the full font is implemented in font6x8.h
	if (chChar == '\xFF')
		chChar = '\x80';

	else if (chChar < ' ')
		chChar = ' ';

	for (u8 i = 0; i < 6; ++i)
	{
		u16 nFontColumn = FontDouble[static_cast<u8>(chChar - ' ')][i];

		// Don't invert the leftmost column or last two rows
		if (i > 0 && bInverted)
			nFontColumn ^= 0x3FFF;

		// Shift down by 2 pixels
		nFontColumn <<= 2;

		PutColumn(pFrameBuffer, nColumnOffset + (bDoubleWidth ? i * 2 : i), nCursorY * 2, nFontColumn & 0xFF);
		PutColumn(pFrameBuffer, nColumnOffset + (bDoubleWidth ? i * 2 : i), nCursorY * 2 + 1, (nFontColumn >> 8) & 0xFF);
		if (bDoubleWidth)
		{
			PutColumn(pFrameBuffer, nColumnOffset + i * 2 + 1, nCursorY * 2, nFontColumn & 0xFF);
			PutColumn(pFrameBuffer, nColumnOffset + i * 2 + 1, nCursorY * 2 + 1, (nFontColumn >> 8) & 0xFF);
		}
	}
}

void CSH1106Device::Flip()
{
	WriteFrameBuffer();
	SwapFrameBuffers();
}

void CSH1106Device::Print(const char* pText, u8 nCursorX, u8 nCursorY, bool bClearLine, bool bImmediate)
{
	if (bClearLine)
	{
		for (u8 nChar = 0; nChar < nCursorX; ++nChar)
			DrawChar(' ', nChar, nCursorY);
	}

	while (*pText && nCursorX < 20)
	{
		DrawChar(*pText++, nCursorX, nCursorY);
		++nCursorX;
	}

	if (bClearLine)
	{
		while (nCursorX < 20)
			DrawChar(' ', nCursorX++, nCursorY);
	}

	if (bImmediate)
		WriteFrameBuffer(true);
}

void CSH1106Device::Clear(bool bImmediate)
{
	u8* pFrameBuffer = m_FrameBuffers[m_nCurrentFrameBuffer].FrameBuffer;
	memset(pFrameBuffer, 0, m_nWidth * m_nHeight / 8);

	if (bImmediate)
		WriteFrameBuffer(true);
}

void CSH1106Device::SetBacklightState(bool bEnabled)
{
	m_bBacklightEnabled = bEnabled;

	// Power on/off display
	WriteCommand(bEnabled ? SetDisplayOn : SetDisplayOff);
}
