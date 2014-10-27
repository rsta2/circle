//
// bcmframebuffer.h
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
#ifndef _bcmframebuffer_h
#define _bcmframebuffer_h

#include <circle/bcmmailbox.h>
#include <circle/types.h>

// This struct is shared between GPU and ARM side
// Must be 16 byte aligned in memory
struct Bcm2835FrameBufferInfo
{
	u32 Width;		// Physical width of display in pixel
	u32 Height;		// Physical height of display in pixel
	u32 VirtWidth;		// always as physical width so far
	u32 VirtHeight;		// always as physical height so far
	u32 Pitch;		// Should be init with 0
	u32 Depth;		// Number of bits per pixel
	u32 OffsetX;		// Normally zero
	u32 OffsetY;		// Normally zero
	u32 BufferPtr;		// Address of frame buffer (init with 0, set by GPU)
	u32 BufferSize;		// Size of frame buffer (init with 0, set by GPU)

	u16 Palette[0];		// with Depth == 8 only (256 entries)
#define PALETTE_ENTRIES		256
};

class CBcmFrameBuffer
{
public:
	CBcmFrameBuffer (unsigned nWidth, unsigned nHeight, unsigned nDepth);
	~CBcmFrameBuffer (void);

	void SetPalette (u8 nIndex, u16 nColor);	// with Depth == 8 only

	boolean Initialize (void);

	u32 GetWidth (void) const;
	u32 GetHeight (void) const;
	u32 GetPitch (void) const;
	u32 GetDepth (void) const;
	u32 GetBuffer (void) const;
	u32 GetSize (void) const;

private:
	volatile Bcm2835FrameBufferInfo *m_pInfo;

	CBcmMailBox m_MailBox;
};

#endif
