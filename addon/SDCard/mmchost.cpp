//
// mmchost.cpp
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
#include <circle/sysconfig.h>
#ifdef USE_SDHOST

#include "mmchost.h"
#include "mmcerror.h"
#include <circle/synchronize.h>
#include <circle/util.h>
#include <assert.h>

CMMCHost::CMMCHost (void)
{
	memset (&m_Data, 0, sizeof m_Data);
}

CMMCHost::~CMMCHost (void)
{
}

void CMMCHost::SetClock (unsigned nClockHz)
{
	m_Data.ios.clock = nClockHz;

	SetIOS (&m_Data.ios);
}

void CMMCHost::SetBusWidth (unsigned nBits)
{
	assert (nBits == 1 || nBits == 4);
	m_Data.ios.bus_width = nBits == 4 ? MMC_BUS_WIDTH_4 : 0;

	SetIOS (&m_Data.ios);
}

int CMMCHost::Command (mmc_command *pCommand, unsigned nRetries)
{
	assert (pCommand != 0);
	pCommand->retries = nRetries;
	memset (pCommand->resp, 0, sizeof pCommand->resp);

	mmc_request Request;
	memset (&Request, 0, sizeof Request);
	Request.cmd = pCommand;
	Request.data = pCommand->data;

	mmc_command StopCmd;
	if (   pCommand->opcode == MMC_READ_MULTIPLE_BLOCK
	    || pCommand->opcode == MMC_WRITE_MULTIPLE_BLOCK)
	{
		assert (pCommand->data != 0);

		memset (&StopCmd, 0, sizeof StopCmd);
		StopCmd.opcode = MMC_STOP_TRANSMISSION;
		StopCmd.flags = MMC_RSP_PRESENT | MMC_RSP_CRC | MMC_RSP_BUSY;

		Request.stop = &StopCmd;
	}

	int nError = PrepareRequest (&Request);
	if (nError != 0)
	{
		return nError;
	}

	RequestSync (&Request);

	if (pCommand->error != 0)
	{
		return pCommand->error;
	}

	if (   pCommand->data != 0
	    && pCommand->data->error != 0)
	{
		return pCommand->data->error;
	}

	if (   Request.stop != 0
	    && Request.stop->error != 0)
	{
		return Request.stop->error;
	}

	return 0;
}

void CMMCHost::RequestSync (mmc_request *pRequest)
{
	assert (pRequest != 0);
	pRequest->done = 0;

	Request (pRequest);

	do
	{
		DataMemBarrier ();
	}
	while (!pRequest->done);
}

int CMMCHost::PrepareRequest (mmc_request *pRequest)
{
	assert (pRequest != 0);

	if (pRequest->cmd)
	{
		pRequest->cmd->error = 0;
		pRequest->cmd->data = pRequest->data;
	}

	if (pRequest->sbc)
	{
		pRequest->sbc->error = 0;
	}

	if (pRequest->data)
	{
		size_t nRequestSize = pRequest->data->blocks * pRequest->data->blksz;

		if (   pRequest->data->blksz > m_Data.max_blk_size
		    || pRequest->data->blocks > m_Data.max_blk_count
		    || nRequestSize > m_Data.max_req_size
		    || nRequestSize != pRequest->data->sg_len)
		{
			return -EINVAL;
		}

		pRequest->data->error = 0;
		pRequest->data->mrq = pRequest;

		if (pRequest->stop)
		{
			pRequest->data->stop = pRequest->stop;
			pRequest->stop->error = 0;
		}
	}

	return 0;
}

void mmc_request_done (mmc_host *mmc, mmc_request *pRequest)
{
	assert (pRequest != 0);
	pRequest->done = 1;

	DataSyncBarrier ();
}

void CMMCHost::sg_miter_start (sg_mapping_iter *sg_miter, void *sg, unsigned sg_len,
			       unsigned flags)
{
	assert (sg_miter != 0);
	assert (sg != 0);
	assert (sg_len > 0);

	sg_miter->addr = sg;
	sg_miter->length = sg_len;
	sg_miter->consumed = 0;
}

bool CMMCHost::sg_miter_next (sg_mapping_iter *sg_miter)
{
	assert (sg_miter != 0);

	assert (sg_miter->consumed <= sg_miter->length);
	sg_miter->length -= sg_miter->consumed;
	if (sg_miter->length == 0)
	{
		return false;
	}

	sg_miter->addr = (u8 *) sg_miter->addr + sg_miter->consumed;
	sg_miter->consumed = 0;

	return true;
}

void CMMCHost::sg_miter_stop (sg_mapping_iter *sg_miter)
{
}

#endif	// #ifdef USE_SDHOST
