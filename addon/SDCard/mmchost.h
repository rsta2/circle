//
// mmchost.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2020  R. Stange <rsta2@o2online.de>
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
#ifndef _SDCard_mmchost_h
#define _SDCard_mmchost_h

#include <circle/types.h>
#include <SDCard/mmc.h>

class CMMCHost
{
public:
	CMMCHost (void);
	virtual ~CMMCHost (void);

	virtual boolean Initialize (void) = 0;

	void SetClock (unsigned nClockHz);
	void SetBusWidth (unsigned nBits);	// 1 or 4 bits

	int Command (mmc_command *pCommand, unsigned nRetries = 0);

	mmc_host *GetMMCHost (void)	{ return &m_Data; }

protected:
	virtual void SetIOS (mmc_ios *pIOS) = 0;
	virtual void Request (mmc_request *pRequest) = 0;
	virtual void Reset (void) = 0;

	static void sg_miter_start (sg_mapping_iter *sg_miter, void *sg, unsigned sg_len,
				    unsigned flags);
	static bool sg_miter_next (sg_mapping_iter *sg_miter);
	static void sg_miter_stop (sg_mapping_iter *sg_miter);

private:
	void RequestSync (mmc_request *pRequest);

	int PrepareRequest (mmc_request *pRequest);

private:
	mmc_host m_Data;
};

#endif
