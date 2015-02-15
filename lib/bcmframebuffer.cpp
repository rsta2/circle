//
// bcmframebuffer.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2015  R. Stange <rsta2@o2online.de>
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
#include <circle/bcmpropertytags.h>
#include <circle/synchronize.h>
#include <circle/bcm2835.h>
#include <circle/util.h>

CBcmFrameBuffer::CBcmFrameBuffer (unsigned nWidth, unsigned nHeight, unsigned nDepth)
:	m_MailBox (MAILBOX_CHANNEL_FB)
{
	if (   nWidth  == 0
	    || nHeight == 0)
	{
		// detect optimal display size (if not configured)
		CBcmPropertyTags Tags;
		TPropertyTagDisplayDimensions Dimensions;
		if (Tags.GetTag (PROPTAG_GET_DISPLAY_DIMENSIONS, &Dimensions, sizeof Dimensions))
		{
			nWidth  = Dimensions.nWidth;
			nHeight = Dimensions.nHeight;
		}
		else
		{
			nWidth  = 640;
			nHeight = 480;
		}
	}

	if (nDepth == 8)
	{
		m_pInfo = (Bcm2835FrameBufferInfo *) new u8 [  sizeof (Bcm2835FrameBufferInfo)
							     + PALETTE_ENTRIES * sizeof (u16)];

		memset ((void *) m_pInfo->Palette, 0, PALETTE_ENTRIES * sizeof (u16));	// clear palette
	}
	else
	{
		m_pInfo = new Bcm2835FrameBufferInfo;
	}

	m_pInfo->Width      = nWidth;
	m_pInfo->Height     = nHeight;
	m_pInfo->VirtWidth  = nWidth;
	m_pInfo->VirtHeight = nHeight;
	m_pInfo->Pitch      = 0;
	m_pInfo->Depth      = nDepth;
	m_pInfo->OffsetX    = 0;
	m_pInfo->OffsetY    = 0;
	m_pInfo->BufferPtr  = 0;
	m_pInfo->BufferSize = 0;
}

CBcmFrameBuffer::~CBcmFrameBuffer (void)
{
	delete m_pInfo;
	m_pInfo = 0;
}

void CBcmFrameBuffer::SetPalette (u8 nIndex, u16 nColor)
{
	if (m_pInfo->Depth == 8)
	{
		m_pInfo->Palette[nIndex] = nColor;
	}
}

boolean CBcmFrameBuffer::Initialize (void)
{
	CleanDataCache ();
	DataSyncBarrier ();
	u32 nResult = m_MailBox.WriteRead (GPU_MEM_BASE + (u32) m_pInfo);
	InvalidateDataCache ();
	
	if (nResult != 0)
	{
		return FALSE;
	}
	
	if (m_pInfo->BufferPtr == 0)
	{
		return FALSE;
	}
	
	return TRUE;
}

unsigned CBcmFrameBuffer::GetWidth (void) const
{
	return m_pInfo->Width;
}

unsigned CBcmFrameBuffer::GetHeight (void) const
{
	return m_pInfo->Height;
}

u32 CBcmFrameBuffer::GetPitch (void) const
{
	return m_pInfo->Pitch;
}

u32 CBcmFrameBuffer::GetDepth (void) const
{
	return m_pInfo->Depth;
}

u32 CBcmFrameBuffer::GetBuffer (void) const
{
#if RASPPI == 1
	return m_pInfo->BufferPtr;
#else
	return m_pInfo->BufferPtr & 0x3FFFFFFF;
#endif
}

u32 CBcmFrameBuffer::GetSize (void) const
{
	return m_pInfo->BufferSize;
}
