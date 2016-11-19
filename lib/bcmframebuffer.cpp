//
// bcmframebuffer.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2016  R. Stange <rsta2@o2online.de>
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
#include <circle/bcmframebuffer.h>
#include <circle/util.h>

static struct
{
	TPropertyTagDisplayDimensions	SetPhysWidthHeight;
	TPropertyTagDisplayDimensions	SetVirtWidthHeight;
	TPropertyTagSimple		SetDepth;
	TPropertyTagVirtualOffset	SetVirtOffset;
	TPropertyTagAllocateBuffer	AllocateBuffer;
	TPropertyTagSimple		GetPitch;
}
InitTags =
{
	{{PROPTAG_SET_PHYS_WIDTH_HEIGHT, 8, 8}},
	{{PROPTAG_SET_VIRT_WIDTH_HEIGHT, 8, 8}},
	{{PROPTAG_SET_DEPTH,		 4, 4}},
	{{PROPTAG_SET_VIRTUAL_OFFSET,	 8, 8}, 0, 0},
	{{PROPTAG_ALLOCATE_BUFFER,	 8, 4}, {0}},
	{{PROPTAG_GET_PITCH,		 4, 0}}
};

CBcmFrameBuffer::CBcmFrameBuffer (unsigned nWidth, unsigned nHeight, unsigned nDepth,
				  unsigned nVirtualWidth, unsigned nVirtualHeight)
:	m_nWidth (nWidth),
	m_nHeight (nHeight),
	m_nVirtualWidth (nVirtualWidth),
	m_nVirtualHeight (nVirtualHeight),
	m_nDepth (nDepth),
	m_nBufferPtr (0),
	m_nBufferSize (0),
	m_nPitch (0),
	m_pTagSetPalette (0)
{
	if (   m_nWidth  == 0
	    || m_nHeight == 0)
	{
		// detect optimal display size (if not configured)
		CBcmPropertyTags Tags;
		TPropertyTagDisplayDimensions Dimensions;
		if (Tags.GetTag (PROPTAG_GET_DISPLAY_DIMENSIONS, &Dimensions, sizeof Dimensions))
		{
			m_nWidth  = Dimensions.nWidth;
			m_nHeight = Dimensions.nHeight;

			if (   m_nWidth  < 640
			    || m_nWidth  > 1920
			    || m_nHeight < 480
			    || m_nHeight > 1080)
			{
				m_nWidth  = 640;
				m_nHeight = 480;
			}
		}
		else
		{
			m_nWidth  = 640;
			m_nHeight = 480;
		}
	}

	if (   m_nVirtualWidth  == 0
	    || m_nVirtualHeight == 0)
	{
		m_nVirtualWidth  = m_nWidth;
		m_nVirtualHeight = m_nHeight;
	}

	if (m_nDepth <= 8)
	{
		m_pTagSetPalette = (TPropertyTagSetPalette *) new u8 [sizeof (TPropertyTagSetPalette)
								      + PALETTE_ENTRIES * sizeof (u32)];

		memset (m_pTagSetPalette->Palette, 0, PALETTE_ENTRIES * sizeof (u32));	// clear palette
	}

	InitTags.SetPhysWidthHeight.nWidth  = m_nWidth;
	InitTags.SetPhysWidthHeight.nHeight = m_nHeight;
	InitTags.SetVirtWidthHeight.nWidth  = m_nVirtualWidth;
	InitTags.SetVirtWidthHeight.nHeight = m_nVirtualHeight;
	InitTags.SetDepth.nValue            = m_nDepth;
}

CBcmFrameBuffer::~CBcmFrameBuffer (void)
{
	delete m_pTagSetPalette;
	m_pTagSetPalette = 0;
}

void CBcmFrameBuffer::SetPalette (u8 nIndex, u16 nRGB565)
{
	if (m_nDepth <= 8)
	{
		u32 nRGBA;
		nRGBA  = (u32) (nRGB565 >> 11 & 0x1F) << (0+3);		// red
		nRGBA |= (u32) (nRGB565 >> 5  & 0x3F) << (8+2);		// green
		nRGBA |= (u32) (nRGB565       & 0x1F) << (16+3);	// blue
		nRGBA |=        0xFF                  << 24;		// alpha

		m_pTagSetPalette->Palette[nIndex] = nRGBA;
	}
}

void CBcmFrameBuffer::SetPalette32 (u8 nIndex, u32 nRGBA)
{
	if (m_nDepth <= 8)
	{
		m_pTagSetPalette->Palette[nIndex] = nRGBA;
	}
}

boolean CBcmFrameBuffer::Initialize (void)
{
	CBcmPropertyTags Tags;
	if (!Tags.GetTags (&InitTags, sizeof InitTags))
	{
		return FALSE;
	}

	if (   InitTags.SetPhysWidthHeight.nWidth         == 0
	    || InitTags.SetPhysWidthHeight.nHeight        == 0
	    || InitTags.SetVirtWidthHeight.nWidth         == 0
	    || InitTags.SetVirtWidthHeight.nHeight        == 0
	    || InitTags.SetDepth.nValue                   == 0
	    || InitTags.AllocateBuffer.nBufferBaseAddress == 0)
	{
		return FALSE;
	}

	m_nBufferPtr  = InitTags.AllocateBuffer.nBufferBaseAddress & 0x3FFFFFFF;
	m_nBufferSize = InitTags.AllocateBuffer.nBufferSize;
	m_nPitch      = InitTags.GetPitch.nValue;

	return UpdatePalette ();
}

unsigned CBcmFrameBuffer::GetWidth (void) const
{
	return m_nWidth;
}

unsigned CBcmFrameBuffer::GetHeight (void) const
{
	return m_nHeight;
}

unsigned CBcmFrameBuffer::GetVirtWidth (void) const
{
	return m_nVirtualWidth;
}

unsigned CBcmFrameBuffer::GetVirtHeight (void) const
{
	return m_nVirtualHeight;
}

u32 CBcmFrameBuffer::GetPitch (void) const
{
	return m_nPitch;
}

u32 CBcmFrameBuffer::GetDepth (void) const
{
	return m_nDepth;
}

u32 CBcmFrameBuffer::GetBuffer (void) const
{
	return m_nBufferPtr;
}

u32 CBcmFrameBuffer::GetSize (void) const
{
	return m_nBufferSize;
}

boolean CBcmFrameBuffer::UpdatePalette (void)
{
	if (m_nDepth <= 8)
	{
		m_pTagSetPalette->nOffset = 0;
		m_pTagSetPalette->nLength = PALETTE_ENTRIES;

		CBcmPropertyTags Tags;
		if (!Tags.GetTag (PROPTAG_SET_PALETTE, m_pTagSetPalette,
				  sizeof *m_pTagSetPalette + PALETTE_ENTRIES*sizeof (u32),
				  (2+PALETTE_ENTRIES) * sizeof (u32)))
		{
			return FALSE;
		}

		if (m_pTagSetPalette->nResult != SET_PALETTE_VALID)
		{
			return FALSE;
		}
	}

	return TRUE;
}

boolean CBcmFrameBuffer::SetVirtualOffset (u32 nOffsetX, u32 nOffsetY)
{
	CBcmPropertyTags Tags;
	TPropertyTagVirtualOffset VirtualOffset;
	VirtualOffset.nOffsetX = nOffsetX;
	VirtualOffset.nOffsetY = nOffsetY;
	if (   !Tags.GetTag (PROPTAG_SET_VIRTUAL_OFFSET, &VirtualOffset, sizeof VirtualOffset, 8)
	    || VirtualOffset.nOffsetX != nOffsetX
	    || VirtualOffset.nOffsetY != nOffsetY)
	{
		return FALSE;
	}

	return TRUE;
}
