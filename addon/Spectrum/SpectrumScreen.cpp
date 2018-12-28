//
// SpectrumScreen.cpp
//
// Spectrum screen emulator code provided by Jose Luis Sanchez
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
#include "SpectrumScreen.h"
#include <circle/util.h>
#include <assert.h>

CSpectrumScreen::CSpectrumScreen (void)
:	m_pFrameBuffer (0),
	m_pBuffer (0),
	m_pVideoMem (0)
{
}

CSpectrumScreen::~CSpectrumScreen (void)
{
	m_pVideoMem = 0;
	m_pBuffer = 0;

	delete m_pFrameBuffer;
	m_pFrameBuffer = 0;
}

boolean CSpectrumScreen::Initialize (u8 *pVideoMem)
{
	m_pVideoMem = pVideoMem;
	assert (m_pVideoMem != 0);

	m_pFrameBuffer = new CBcmFrameBuffer (344, 272, 4);
	assert (m_pFrameBuffer != 0);

	// Color palette definition, uses RGB565 format
	// Needs to be defined BEFORE call Initialize
	m_pFrameBuffer->SetPalette (0, 0x0000);  // black
	m_pFrameBuffer->SetPalette (1, 0x0010);  // blue
	m_pFrameBuffer->SetPalette (2, 0x8000);  // red
	m_pFrameBuffer->SetPalette (3, 0x8010);  // magenta
	m_pFrameBuffer->SetPalette (4, 0x0400);  // green
	m_pFrameBuffer->SetPalette (5, 0x0410);  // cyan
	m_pFrameBuffer->SetPalette (6, 0x8400);  // yellow
	m_pFrameBuffer->SetPalette (7, 0x8410);  // white
	m_pFrameBuffer->SetPalette (8, 0x0000);  // black
	m_pFrameBuffer->SetPalette (9, 0x001F);  // bright blue
	m_pFrameBuffer->SetPalette (10, 0xF800); // bright red
	m_pFrameBuffer->SetPalette (11, 0xF81F); // bright magenta
	m_pFrameBuffer->SetPalette (12, 0x07E0); // bright green
	m_pFrameBuffer->SetPalette (13, 0x07FF); // bright cyan
	m_pFrameBuffer->SetPalette (14, 0xFFE0); // bright yellow
	m_pFrameBuffer->SetPalette (15, 0xFFFF); // bright white

	if (!m_pFrameBuffer->Initialize()) {
		return FALSE;
	}

	m_pBuffer = (u32 *) (uintptr) m_pFrameBuffer->GetBuffer();
	assert (m_pBuffer != 0);

	memset(m_pBuffer, 0xFF, m_pFrameBuffer->GetSize());

	// Create a lookup table for draw the screen Faster Than Light :)
	for (int attr = 0; attr < 128; attr++) {
		unsigned ink = attr & 0x07;
		unsigned paper = (attr & 0x78) >> 3;
		if (attr & 0x40) {
			ink |= 0x08;
		}
		for (int idx = 0; idx < 256; idx++) {
			for (unsigned mask = 0x80; mask > 0; mask >>= 1) {
				m_scrTable[attr][idx] <<= 4;
				m_scrTable[attr + 128][idx] <<= 4;
				if (idx & mask) {
					m_scrTable[attr][idx] |= ink;
					m_scrTable[attr + 128][idx] |= paper;
				} else {
					m_scrTable[attr][idx] |= paper;
					m_scrTable[attr + 128][idx] |= ink;
				}
			}
			m_scrTable[attr][idx] = bswap32(m_scrTable[attr][idx]);
			m_scrTable[attr + 128][idx] = bswap32(m_scrTable[attr + 128][idx]);
		}
	}

	return TRUE;
}

void CSpectrumScreen::Update (boolean flash)
{
	assert (m_pBuffer != 0);
	assert (m_pVideoMem != 0);

	int bufIdx = 1413;
	int attr = 6144;
	for (int addr = 0; addr < 256; addr += 32) {
		for (int col = 0; col < 32; col++) {
			unsigned char color = m_pVideoMem[attr++];
			if (!flash)
				color &= 0x7F;
			m_pBuffer[bufIdx + col] = m_scrTable[color][m_pVideoMem[addr + col]];
			m_pBuffer[bufIdx + col + 44] = m_scrTable[color][m_pVideoMem[addr + col + 256]];
			m_pBuffer[bufIdx + col + 88] = m_scrTable[color][m_pVideoMem[addr + col + 512]];
			m_pBuffer[bufIdx + col + 132] = m_scrTable[color][m_pVideoMem[addr + col + 768]];
			m_pBuffer[bufIdx + col + 176] = m_scrTable[color][m_pVideoMem[addr + col + 1024]];
			m_pBuffer[bufIdx + col + 220] = m_scrTable[color][m_pVideoMem[addr + col + 1280]];
			m_pBuffer[bufIdx + col + 264] = m_scrTable[color][m_pVideoMem[addr + col + 1536]];
			m_pBuffer[bufIdx + col + 308] = m_scrTable[color][m_pVideoMem[addr + col + 1792]];
		}
		bufIdx += 352;
	}
	for (int addr = 2048; addr < 2304; addr += 32) {
		for (int col = 0; col < 32; col++) {
			unsigned char color = m_pVideoMem[attr++];
			if (!flash)
				color &= 0x7F;
			m_pBuffer[bufIdx + col] = m_scrTable[color][m_pVideoMem[addr + col]];
			m_pBuffer[bufIdx + col + 44] = m_scrTable[color][m_pVideoMem[addr + col + 256]];
			m_pBuffer[bufIdx + col + 88] = m_scrTable[color][m_pVideoMem[addr + col + 512]];
			m_pBuffer[bufIdx + col + 132] = m_scrTable[color][m_pVideoMem[addr + col + 768]];
			m_pBuffer[bufIdx + col + 176] = m_scrTable[color][m_pVideoMem[addr + col + 1024]];
			m_pBuffer[bufIdx + col + 220] = m_scrTable[color][m_pVideoMem[addr + col + 1280]];
			m_pBuffer[bufIdx + col + 264] = m_scrTable[color][m_pVideoMem[addr + col + 1536]];
			m_pBuffer[bufIdx + col + 308] = m_scrTable[color][m_pVideoMem[addr + col + 1792]];
		}
		bufIdx += 352;
	}
	for (int addr = 4096; addr < 4352; addr += 32) {
		for (int col = 0; col < 32; col++) {
			unsigned char color = m_pVideoMem[attr++];
			if (!flash)
				color &= 0x7F;
			m_pBuffer[bufIdx + col] = m_scrTable[color][m_pVideoMem[addr + col]];
			m_pBuffer[bufIdx + col + 44] = m_scrTable[color][m_pVideoMem[addr + col + 256]];
			m_pBuffer[bufIdx + col + 88] = m_scrTable[color][m_pVideoMem[addr + col + 512]];
			m_pBuffer[bufIdx + col + 132] = m_scrTable[color][m_pVideoMem[addr + col + 768]];
			m_pBuffer[bufIdx + col + 176] = m_scrTable[color][m_pVideoMem[addr + col + 1024]];
			m_pBuffer[bufIdx + col + 220] = m_scrTable[color][m_pVideoMem[addr + col + 1280]];
			m_pBuffer[bufIdx + col + 264] = m_scrTable[color][m_pVideoMem[addr + col + 1536]];
			m_pBuffer[bufIdx + col + 308] = m_scrTable[color][m_pVideoMem[addr + col + 1792]];
		}
		bufIdx += 352;
	}
}
