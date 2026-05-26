//
// tc358762dsibridge.h
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
#ifndef _rp1dsi_tc358762dsibridge_h
#define _rp1dsi_tc358762dsibridge_h

#include "drm_mipi_dsi.h"
#include "attinyregulator.h"
#include "drmcompat.h"
#include <circle/types.h>

class CTC358762DSIBridge	/// MIPI-DSI based Driver for TC358762 DSI Bridge
{
public:
	CTC358762DSIBridge (mipi_dsi_device *pDevice,
			    CATTinyRegulator *pRegulator,
			    const drm_display_mode *pMode);
	~CTC358762DSIBridge (void);

	boolean Initialize (void);

private:
	int tc358762_init (void);

	void tc358762_write (u16 addr, u32 val);

	int tc358762_clear_error (void);

private:
	mipi_dsi_device *m_pDevice;
	CATTinyRegulator *m_pRegulator;
	const drm_display_mode *m_pMode;

	int m_error;
};

#endif
