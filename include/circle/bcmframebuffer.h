//
// bcmframebuffer.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_bcmframebuffer_h
#define _circle_bcmframebuffer_h

#include <circle/bcmpropertytags.h>
#include <circle/types.h>

#define PALETTE_ENTRIES		256

struct TBcmFrameBufferInitTags
{
	TPropertyTagDisplayDimensions	SetPhysWidthHeight;
	TPropertyTagDisplayDimensions	SetVirtWidthHeight;
	TPropertyTagSimple		SetDepth;
	TPropertyTagVirtualOffset	SetVirtOffset;
	TPropertyTagAllocateBuffer	AllocateBuffer;
	TPropertyTagSimple		GetPitch;
};

class CBcmFrameBuffer
{
public:
	CBcmFrameBuffer (unsigned nWidth, unsigned nHeight, unsigned nDepth,
			 unsigned nVirtualWidth = 0, unsigned nVirtualHeight = 0,
			 unsigned nDisplay = 0, boolean bDoubleBuffered = FALSE);
	~CBcmFrameBuffer (void);

	void SetPalette (u8 nIndex, u16 nRGB565);	// with Depth <= 8 only
	void SetPalette32 (u8 nIndex, u32 nRGBA);	// with Depth <= 8 only

	boolean Initialize (void);

	u32 GetWidth (void) const;
	u32 GetHeight (void) const;
	u32 GetVirtWidth(void) const;
	u32 GetVirtHeight(void) const;
	u32 GetPitch (void) const;
	u32 GetDepth (void) const;
	u32 GetBuffer (void) const;
	u32 GetSize (void) const;

	boolean UpdatePalette (void);			// with Depth <= 8 only

	boolean SetVirtualOffset (u32 nOffsetX, u32 nOffsetY);

	boolean WaitForVerticalSync (void);

	/// \brief Set screen backlight brightness
	/// \param nBrightness Brightness level
	/// \return Operation successful?
	/// \note Tested with the Official Touchscreen only (level can be about 0..180)
	boolean SetBacklightBrightness(unsigned nBrightness);

	static unsigned GetNumDisplays (void);

private:
	void SetDisplay (void);

private:
	u32 m_nWidth;
	u32 m_nHeight;
	u32 m_nVirtualWidth;
	u32 m_nVirtualHeight;
	u32 m_nDepth;
	u32 m_nDisplay;

	u32 m_nBufferPtr;
	u32 m_nBufferSize;
	u32 m_nPitch;

	TPropertyTagSetPalette *m_pTagSetPalette;	// with Depth <= 8 only (256 entries)

	TBcmFrameBufferInitTags m_InitTags;
	static const TBcmFrameBufferInitTags s_InitTags;

#if RASPPI >= 4
	static unsigned s_nDisplays;
	static unsigned s_nCurrentDisplay;
#endif
};

#endif
