//
// timeline.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016  R. Stange <rsta2@o2online.de>
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
#include "timeline.h"
#include <assert.h>

#define MAX_ZOOM	32.0
#define MIN_ZOOM	(1.0 / MAX_ZOOM)

CTimeLine::CTimeLine (unsigned nMemoryDepth, double fRuntime, unsigned nWindowSizePixel)
:	m_nMemoryDepth (nMemoryDepth),
	m_fRuntime (fRuntime),
	m_nWindowSizePixel (nWindowSizePixel),
	m_fWindowLeft (0.0)
{
	assert (m_nMemoryDepth > 0);
	assert (m_fRuntime > 0.0);
	assert (m_nWindowSizePixel > 0);

	m_fSampleRate = (double) m_nMemoryDepth / m_fRuntime;
	m_fMeasureDuration = 1.0 / m_fSampleRate;

	SetZoom (TIMELINE_ZOOM_DEFAULT);
	SetOffset (0.0);
}

CTimeLine::~CTimeLine (void)
{
}

void CTimeLine::SetZoom (int nZoom)
{
	switch (nZoom)
	{
	case TIMELINE_ZOOM_DEFAULT:
		m_fZoomFactor = 1.0;
		break;

	case TIMELINE_ZOOM_OUT:
		if (m_fZoomFactor > MIN_ZOOM)
		{
			m_fZoomFactor /= 2.0;
		}
		break;

	case TIMELINE_ZOOM_IN:
		if (m_fZoomFactor < MAX_ZOOM)
		{
			m_fZoomFactor *= 2.0;
		}
		break;

	default:
		assert (0);
		break;
	}

	m_fPixelDuration  = m_fMeasureDuration / m_fZoomFactor;
	m_fWindowDuration = m_fPixelDuration * (double) m_nWindowSizePixel;
	m_fMaxOffset      = m_fRuntime - m_fWindowDuration;

	m_fScaleDivision = m_fPixelDuration * 100.0;
	m_fScaleDivision = (unsigned) (m_fScaleDivision * 100000.0 + 0.5) / 100000.0;
	if (m_fScaleDivision < 1e-6)
	{
		m_fScaleDivision = 1e-6;
	}

	if (m_fWindowLeft > m_fMaxOffset)
	{
		m_fWindowLeft = m_fMaxOffset;
	}

	m_fWindowRight = m_fWindowLeft + m_fWindowDuration;
}

void CTimeLine::SetOffset (double fOffset)
{
	if (fOffset > m_fMaxOffset)
	{
		fOffset = m_fMaxOffset;
	}

	m_fWindowLeft = fOffset;
	m_fWindowRight = m_fWindowLeft + m_fWindowDuration;
}

void CTimeLine::AddOffset (int nPixel)
{
	m_fWindowLeft += nPixel * m_fPixelDuration;

	if (m_fWindowLeft < 0.0)
	{
		m_fWindowLeft = 0.0;
	}
	else if (m_fWindowLeft > m_fMaxOffset)
	{
		m_fWindowLeft = m_fMaxOffset;
	}

	m_fWindowRight = m_fWindowLeft + m_fWindowDuration;
}

double CTimeLine::GetScaleDivision (void) const
{
	return m_fScaleDivision;
}

double CTimeLine::GetRuntime (void) const
{
	return m_fRuntime;
}

double CTimeLine::GetZoomFactor (void) const
{
	return m_fZoomFactor;
}

double CTimeLine::GetSampleRate (void) const
{
	return m_fSampleRate;
}

double CTimeLine::GetWindowLeft (void) const
{
	return m_fWindowLeft;
}

double CTimeLine::GetWindowRight (void) const
{
	return m_fWindowRight;
}

double CTimeLine::GetPixelDuration (void) const
{
	return m_fPixelDuration;
}

unsigned CTimeLine::GetMeasure (double fTime) const
{
	if (fTime > m_fRuntime)
	{
		return TIMELINE_INVALID_MEASURE;
	}

	return (unsigned) (fTime / m_fMeasureDuration);
}

unsigned CTimeLine::GetPixel (double fTime) const
{
	if (   m_fWindowLeft  > fTime
	    || m_fWindowRight < fTime)
	{
		return TIMELINE_INVALID_PIXEL;
	}

	return (unsigned) ((fTime-m_fWindowLeft) / m_fPixelDuration);
}
