//
// screen.h
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
#ifndef _screen_h
#define _screen_h

#include <circle/bcmframebuffer.h>
#include <circle/types.h>

#define DEPTH	8		// can be: 8, 16 or 32

// really ((green) & 0x3F) << 5, but to have a 0-31 range for all colors
#define COLOR16(red, green, blue)	  (((red) & 0x1F) << 11 \
					| ((green) & 0x1F) << 6 \
					| ((blue) & 0x1F))

#define COLOR32(red, green, blue, alpha)  (((red) & 0xFF)        \
					| ((green) & 0xFF) << 8  \
					| ((blue) & 0xFF) << 16  \
					| ((alpha) & 0xFF) << 24)

#define BLACK_COLOR	0

#if DEPTH == 8
	typedef u8 TScreenColor;

	#define NORMAL_COLOR16			COLOR16 (31, 31, 31)
	#define HIGH_COLOR16			COLOR16 (31, 0, 0)
	#define HALF_COLOR16			COLOR16 (0, 0, 31)

	#define NORMAL_COLOR			1
	#define HIGH_COLOR			2
	#define HALF_COLOR			3
#elif DEPTH == 16
	typedef u16 TScreenColor;

	#define NORMAL_COLOR			COLOR16 (31, 31, 31)
	#define HIGH_COLOR			COLOR16 (31, 0, 0)
	#define HALF_COLOR			COLOR16 (0, 0, 31)
#elif DEPTH == 32
	typedef u32 TScreenColor;

	#define NORMAL_COLOR			COLOR32 (255, 255, 255, 255)
	#define HIGH_COLOR			COLOR32 (255, 0, 0, 255)
	#define HALF_COLOR			COLOR32 (0, 0, 255, 255)
#else
	#error DEPTH must be 8, 16 or 32
#endif

class CScreenDevice
{
public:
	CScreenDevice (unsigned nWidth, unsigned nHeight);
	~CScreenDevice (void);

	boolean Initialize (void);

	unsigned GetWidth (void) const;
	unsigned GetHeight (void) const;
	
	void SetPixel (unsigned nPosX, unsigned nPosY, TScreenColor Color);
	TScreenColor GetPixel (unsigned nPosX, unsigned nPosY);

private:
	unsigned	 m_nInitWidth;
	unsigned	 m_nInitHeight;
	CBcmFrameBuffer	*m_pFrameBuffer;
	TScreenColor  	*m_pBuffer;
	unsigned	 m_nSize;
	unsigned	 m_nWidth;
	unsigned	 m_nHeight;
};

#endif
