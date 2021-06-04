//
// rpitouchscreen.cpp
//
// This is based on the the Raspbian Linux driver:
//
//   drivers/input/touchscreen/rpi-ft5406.c
//
//   Driver for memory based ft5406 touchscreen
//
//   Copyright (C) 2015 Raspberry Pi
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.
//
#include <circle/input/rpitouchscreen.h>
#include <circle/bcmpropertytags.h>
#include <circle/memory.h>
#include <circle/bcm2835.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <assert.h>

struct TFT5406Buffer
{
	u8	DeviceMode;
	u8	GestureID;
	u8	NumPoints;

	struct
	{
		u8	xh;
		u8	xl;
		u8	yh;
		u8	yl;
		u8	Reserved[2];
	}
	Point[RPITOUCH_SCREEN_MAX_POINTS];
};

#define FTS_TOUCH_DOWN		0
#define FTS_TOUCH_UP		1
#define FTS_TOUCH_CONTACT	2

static const char FromFT5406[] = "ft5406";

CRPiTouchScreen::CRPiTouchScreen (void)
:	m_pFT5406Buffer (0),
	m_nKnownIDs (0),
	m_pDevice (0)
{
}

CRPiTouchScreen::~CRPiTouchScreen (void)
{
	delete m_pDevice;
	m_pDevice = 0;

	m_pFT5406Buffer = 0;
}

boolean CRPiTouchScreen::Initialize (void)
{
	assert (m_pFT5406Buffer == 0);

	uintptr nTouchBuffer = CMemorySystem::GetCoherentPage (COHERENT_SLOT_TOUCHBUF);

	CBcmPropertyTags Tags;
	TPropertyTagSimple TagSimple;
	TagSimple.nValue = BUS_ADDRESS (nTouchBuffer);
	if (!Tags.GetTag (PROPTAG_SET_TOUCHBUF, &TagSimple, sizeof TagSimple))
	{
		if (!Tags.GetTag (PROPTAG_GET_TOUCHBUF, &TagSimple, sizeof TagSimple))
		{
			CLogger::Get ()->Write (FromFT5406, LogError, "Cannot get touch buffer");

			return FALSE;
		}

		if (TagSimple.nValue == 0)
		{
			CLogger::Get ()->Write (FromFT5406, LogError, "Touchscreen not detected");

			return FALSE;
		}

		nTouchBuffer = TagSimple.nValue & ~0xC0000000;
	}

	m_pFT5406Buffer = (TFT5406Buffer *) nTouchBuffer;
	assert (m_pFT5406Buffer != 0);
	*(volatile u8 *) &m_pFT5406Buffer->NumPoints = 99;

	assert (m_pDevice == 0);
	m_pDevice = new CTouchScreenDevice (UpdateStub, this);
	assert (m_pDevice != 0);

	return TRUE;
}

void CRPiTouchScreen::Update (void)
{
	TFT5406Buffer Regs;
	assert (m_pFT5406Buffer != 0);
	memcpy (&Regs, m_pFT5406Buffer, sizeof *m_pFT5406Buffer);
	*(volatile u8 *) &m_pFT5406Buffer->NumPoints = 99;

	// Do not output if theres no new information (NumPoints is 99)
	// or we have no touch points and don't need to release any
	if (   Regs.NumPoints == 99
	    || (   Regs.NumPoints == 0
		&& m_nKnownIDs == 0))
	{
		return;
	}

	assert (m_pDevice != 0);

	unsigned nModifiedIDs = 0;
	assert (Regs.NumPoints <= RPITOUCH_SCREEN_MAX_POINTS);
	for (unsigned i = 0; i < Regs.NumPoints; i++)
	{
		unsigned x = (((unsigned) Regs.Point[i].xh & 0xF) << 8) | Regs.Point[i].xl;
		unsigned y = (((unsigned) Regs.Point[i].yh & 0xF) << 8) | Regs.Point[i].yl;

		unsigned nTouchID = (Regs.Point[i].yh >> 4) & 0xF;
		unsigned nEventID = (Regs.Point[i].xh >> 6) & 0x3;
		assert (nTouchID < RPITOUCH_SCREEN_MAX_POINTS);

		nModifiedIDs |= 1 << nTouchID;

		if (nEventID == FTS_TOUCH_CONTACT || nEventID == FTS_TOUCH_DOWN)
		{
			if (!((1 << nTouchID) & m_nKnownIDs))
			{
				m_nPosX[nTouchID] = x;
				m_nPosY[nTouchID] = y;

				m_pDevice->ReportHandler (TouchScreenEventFingerDown, nTouchID, x, y);
			}
			else
			{
				if (   x != m_nPosX[nTouchID]
				    || y != m_nPosY[nTouchID])
				{
					m_nPosX[nTouchID] = x;
					m_nPosY[nTouchID] = y;

					m_pDevice->ReportHandler (TouchScreenEventFingerMove,
								  nTouchID, x, y);
				}
			}
		}
	}

	unsigned nReleasedIDs = m_nKnownIDs & ~nModifiedIDs;
	for (unsigned i = 0; nReleasedIDs != 0 && i < RPITOUCH_SCREEN_MAX_POINTS; i++)
	{
		if (nReleasedIDs & (1 << i))
		{
			m_pDevice->ReportHandler (TouchScreenEventFingerUp, i, 0, 0);

			nModifiedIDs &= ~(1 << i);
		}
	}

	m_nKnownIDs = nModifiedIDs;
}

void CRPiTouchScreen::UpdateStub (void *pParam)
{
	CRPiTouchScreen *pThis = static_cast<CRPiTouchScreen *> (pParam);
	assert (pThis != 0);

	pThis->Update ();
}
