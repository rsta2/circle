//
// rpitouchscreen.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2026  R. Stange <rsta2@gmx.net>
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
#include "rpitouchscreen.h"
#include "linuxcompat.h"
#include "ili9881panel.h"
#include <circle/synchronize.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <assert.h>

LOGMODULE ("rp1touch");

CRPiTouchScreen::CRPiTouchScreen (CInterruptSystem *pInterrupt,
				  unsigned nDepth, unsigned nDisplay,
				  boolean bEnableTouch)
:	CDisplay (nDepth == 16 ? RGB565 : ARGB8888),
	m_pInterrupt (pInterrupt),
	m_nDepth (nDepth),
	m_nDisplay (nDisplay),
	m_bEnableTouch (bEnableTouch),
	m_nRotation (0),
	m_nWidth (0),
	m_nHeight (0),
	m_nPitch (0),
	m_pFrameBuffer (nullptr),
	m_I2C (nDisplay ? 4 : 6, FALSE),
	m_pTouchScreen1 (nullptr),
	m_pTouchScreen2 (nullptr)
{
	assert (nDisplay <= 1);
}

CRPiTouchScreen::~CRPiTouchScreen (void)
{
	delete m_pTouchScreen1;
	m_pTouchScreen1 = nullptr;

	delete m_pTouchScreen2;
	m_pTouchScreen2 = nullptr;

	delete [] m_pFrameBuffer;
	m_pFrameBuffer = nullptr;
}

boolean CRPiTouchScreen::Initialize (void)
{
	if (!m_I2C.Initialize ())
	{
		LOGERR ("Cannot init I2C master");
		return FALSE;
	}

	u8 uchID = GetRegID ();
	if (uchID == 0xFF)
	{
		LOGERR ("Cannot read regulator ID");
		return FALSE;
	}

	assert (!m_pTouchScreen1);
	assert (!m_pTouchScreen2);

	switch (uchID & 0x0F)
	{
	case 0x01: /* 7 inch */
	case 0x04: /* 7 inch - old */
		m_pTouchScreen2 = new CRPiTouchScreen2 (m_pInterrupt, &m_I2C, m_nDepth, m_nDisplay,
							m_bEnableTouch,
							CILI9881Panel::RPi7InchPanel);
		break;

	case 0x08: /* 5 inch - old */
	case 0x09: /* 5 inch */
		m_pTouchScreen2 = new CRPiTouchScreen2 (m_pInterrupt, &m_I2C, m_nDepth, m_nDisplay,
							m_bEnableTouch,
							CILI9881Panel::RPi5InchPanel);
		break;

	default:
		m_pTouchScreen1 = new CRPiTouchScreen1 (m_pInterrupt, &m_I2C, m_nDepth, m_nDisplay,
						        m_bEnableTouch);
		break;
	}

	if (m_pTouchScreen1)
	{
		m_pTouchScreen1->SetRotation (m_nRotation);

		if (!m_pTouchScreen1->Initialize ())
		{
			LOGERR ("Cannot init v1 touch screen");
			return FALSE;
		}

		m_nWidth = m_pTouchScreen1->GetWidth ();
		m_nHeight = m_pTouchScreen1->GetHeight ();
	}
	else if (m_pTouchScreen2)
	{
		if (!m_pTouchScreen2->Initialize ())
		{
			LOGERR ("Cannot init v2 touch screen");
			return FALSE;
		}

		m_nWidth = m_pTouchScreen2->GetWidth ();
		m_nHeight = m_pTouchScreen2->GetHeight ();
	}
	else
	{
		LOGERR ("Unknown revision (0x%02X)", (unsigned) uchID);
		return FALSE;
	}

	assert (m_nDepth == 16 || m_nDepth == 32);
	m_nPitch = m_nWidth * (m_nDepth / 8);
	size_t nSize = m_nPitch * m_nHeight;

	assert (!m_pFrameBuffer);
	m_pFrameBuffer = new u8[nSize];
	if (!m_pFrameBuffer)
	{
		LOGERR ("Cannot allocate frame buffer");
		return FALSE;
	}

	memset (m_pFrameBuffer, 0, nSize);
	CleanAndInvalidateDataCacheRange (reinterpret_cast<uintptr> (m_pFrameBuffer), nSize);

	if (m_pTouchScreen1)
	{
		if (!m_pTouchScreen1->Start (m_pFrameBuffer, m_nPitch))
		{
			LOGERR ("Cannot start v1 touch screen");
			return FALSE;
		}
	}
	else
	{
		if (!m_pTouchScreen2->Start (m_pFrameBuffer, m_nPitch))
		{
			LOGERR ("Cannot start v2 touch screen");
			return FALSE;
		}
	}
	msleep (100);

	if (!SetBacklightBrightness (200))
	{
		LOGERR ("Cannot set backlight on");
		return FALSE;
	}

	return TRUE;
}

void CRPiTouchScreen::SetPixel (unsigned nPosX, unsigned nPosY, TRawColor nColor)
{
	assert (m_pFrameBuffer);

	if (m_nDepth == 16)
	{
		u16 *p = reinterpret_cast<u16 *> (m_pFrameBuffer);
		p += nPosY*m_nWidth + nPosX;
		*p = nColor;

		CleanAndInvalidateDataCacheRange (reinterpret_cast<uintptr> (p), sizeof (u16));
	}
	else
	{
		assert (m_nDepth == 32);

		u32 *p = reinterpret_cast<u32 *> (m_pFrameBuffer);
		p += nPosY*m_nWidth + nPosX;
		*p = nColor;

		CleanAndInvalidateDataCacheRange (reinterpret_cast<uintptr> (p), sizeof (u32));
	}
}

#define PTR_ADD(type, ptr, items, bytes)	((type) ((uintptr) (ptr) + (bytes)) + (items))

void CRPiTouchScreen::SetArea (const TArea &rArea, const void *pPixels,
				TAreaCompletionRoutine *pRoutine, void *pParam)
{
	size_t ulWidth = rArea.x2 - rArea.x1 + 1;
	size_t ulBlockLength = ulWidth * m_nDepth/8;

	switch (m_nDepth)
	{
	case 16: {
			auto p = PTR_ADD (u16 *, m_pFrameBuffer, rArea.x1, rArea.y1 * m_nPitch);
			auto q = static_cast<const u16 *> (pPixels);

			for (unsigned y = rArea.y1; y <= rArea.y2; y++, q += ulWidth)
			{
				memcpy (p, q, ulBlockLength);

				CleanAndInvalidateDataCacheRange (reinterpret_cast<uintptr> (p),
								  ulBlockLength);

				p = PTR_ADD (u16 *, p, 0, m_nPitch);
			}
		} break;

	case 32: {
			auto p = PTR_ADD (u32 *, m_pFrameBuffer, rArea.x1, rArea.y1 * m_nPitch);
			auto q = static_cast<const u32 *> (pPixels);

			for (unsigned y = rArea.y1; y <= rArea.y2; y++, q += ulWidth)
			{
				memcpy (p, q, ulBlockLength);

				CleanAndInvalidateDataCacheRange (reinterpret_cast<uintptr> (p),
								  ulBlockLength);

				p = PTR_ADD (u32 *, p, 0, m_nPitch);
			}
		} break;
	}

	if (pRoutine)
	{
		(*pRoutine) (pParam);
	}
}

boolean CRPiTouchScreen::SetBacklightBrightness (unsigned nBrightness)
{
	if (m_pTouchScreen1)
	{
		return m_pTouchScreen1->SetBacklightBrightness (nBrightness);
	}
	else
	{
		return m_pTouchScreen2->SetBacklightBrightness (nBrightness);
	}
}

void CRPiTouchScreen::RegisterVerticalSyncHandler (TVerticalSyncHandler *pHandler, void *pParam)
{
	if (m_pTouchScreen1)
	{
		m_pTouchScreen1->RegisterVerticalSyncHandler (pHandler, pParam);
	}
	else
	{
		m_pTouchScreen2->RegisterVerticalSyncHandler (pHandler, pParam);
	}
}

#define REG_ID		0x01

u8 CRPiTouchScreen::GetRegID (void)
{
	/* Write register address */
	const u8 reg = REG_ID;
	int ret = m_I2C.Write (I2CAddress, &reg, 1);
	if (ret != 1)
		return 0xFF;

	usleep_range(5000, 10000);

	/* Read data from register */
	u8 buf;
	ret = m_I2C.Read (I2CAddress, &buf, 1);
	if (ret != 1)
		return 0xFF;

	return buf;
}
