//
// timeline.h
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
#ifndef _timeline_h
#define _timeline_h

class CTimeLine					// all times in seconds
{
public:
	CTimeLine (unsigned nMemoryDepth, double fRuntime, unsigned nWindowSizePixel);
	~CTimeLine (void);

	void SetZoom (int nZoom);
#define TIMELINE_ZOOM_DEFAULT		0
#define TIMELINE_ZOOM_OUT		-1
#define TIMELINE_ZOOM_IN		1
	void SetOffset (double fOffset);
#define TIMELINE_MAX_OFFSET		100000.0
	void AddOffset (int nPixel);

	double GetScaleDivision (void) const;
	double GetRuntime (void) const;
	double GetZoomFactor (void) const;
	double GetSampleRate (void) const;	// in Hz

	double GetWindowLeft (void) const;
	double GetWindowRight (void) const;
	double GetPixelDuration (void) const;

	unsigned GetMeasure (double fTime) const;
#define TIMELINE_INVALID_MEASURE	((unsigned) -1)
	unsigned GetPixel (double fTime) const;
#define TIMELINE_INVALID_PIXEL		((unsigned) -1)

private:
	unsigned m_nMemoryDepth;
	double	 m_fRuntime;
	unsigned m_nWindowSizePixel;

	double	 m_fSampleRate;
	double	 m_fMeasureDuration;

	double	 m_fZoomFactor;
	double	 m_fPixelDuration;
	double	 m_fWindowDuration;
	double	 m_fMaxOffset;
	double	 m_fScaleDivision;

	double	 m_fWindowLeft;
	double	 m_fWindowRight;
};

#endif
