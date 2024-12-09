//
/// \file touchscreen.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2024  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_input_touchscreen_h
#define _circle_input_touchscreen_h

#include <circle/device.h>
#include <circle/display.h>
#include <circle/numberpool.h>
#include <circle/types.h>

enum TTouchScreenEvent
{
	TouchScreenEventFingerDown,
	TouchScreenEventFingerUp,
	TouchScreenEventFingerMove,
	TouchScreenEventUnknown
};

/// \brief For internal use only
typedef void TTouchScreenUpdateHandler (void *pParam);

/// \brief Handler to be called on a touch screen event
typedef void TTouchScreenEventHandler (TTouchScreenEvent Event,
				       unsigned nID, unsigned nPosX, unsigned nPosY);

class CTouchScreenDevice : public CDevice	/// Generic touch screen interface device
{
public:
	CTouchScreenDevice (TTouchScreenUpdateHandler *pUpdateHandler = 0, void *pParam = 0);
	~CTouchScreenDevice (void);

	/// \param pDisplay The display (or window), which receives the touch events
	/// \note Must be called before any other method!
	void Setup (CDisplay *pDisplay);

	/// \note Call this about 60 times per second!
	void Update (void);

	void RegisterEventHandler (TTouchScreenEventHandler *pEventHandler);

	/// \param Coords Usable Touch screen coordinates (min x, max x, min y, max y)
	/// \return Calibration data valid?
	boolean SetCalibration (const unsigned Coords[4]);

public:
	/// \warning Do not call this from application!
	void ReportHandler (TTouchScreenEvent Event, unsigned nID, unsigned nPosX, unsigned nPosY);

private:
	TTouchScreenUpdateHandler *m_pUpdateHandler;
	void *m_pUpdateParam;

	CDisplay *m_pDisplay;
	CDisplay *m_pParent;

	TTouchScreenEventHandler *m_pEventHandler;

	boolean m_bIsCalibrated;
	unsigned m_nScaleX;	// scale * 1000
	unsigned m_nScaleY;	// scale * 1000
	unsigned m_nOffsetX;
	unsigned m_nOffsetY;

	unsigned m_nDeviceNumber;
	static CNumberPool s_DeviceNumberPool;
};

#endif
