//
// ili9881panel.h
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
#ifndef _rp1dsi_ili9881panel_h
#define _rp1dsi_ili9881panel_h

#include "drm_mipi_dsi.h"
#include "rpipanel2regulator.h"
#include "drmcompat.h"
#include <circle/types.h>

class CILI9881Panel	/// Ilitek ILI9881C Controller Driver
{
public:
	enum TPanelType
	{
		RPi5InchPanel,
		RPi7InchPanel,
		PanelUnknown
	};

public:
	CILI9881Panel (mipi_dsi_device *pDevice,
		       CRPiPanel2Regulator *pRegulator);
	~CILI9881Panel (void);

	boolean Initialize (TPanelType PanelType);

	static const drm_display_mode *GetDisplayMode (TPanelType PanelType);

private:
	int ili9881c_prepare (void);
	int ili9881c_unprepare (void);

	int ili9881c_switch_page (u8 page);
	int ili9881c_send_cmd_data (u8 cmd, u8 data);

private:
	mipi_dsi_device *m_pDevice;
	CRPiPanel2Regulator *m_pRegulator;

	TPanelType m_PanelType;

	boolean m_bInitialized;
};

#endif
