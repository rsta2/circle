//
// dwhcixferstagedata.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2015  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/dwhcixferstagedata.h>
#include <circle/usb/dwhciframeschedper.h>
#include <circle/usb/dwhciframeschednper.h>
#include <circle/usb/dwhciframeschednsplit.h>
#include <circle/usb/dwhci.h>
#include <circle/logger.h>
#include <assert.h>

CDWHCITransferStageData::CDWHCITransferStageData (unsigned	 nChannel,
						  CUSBRequest	*pURB,
						  boolean	 bIn,
						  boolean	 bStatusStage)
:	m_nChannel (nChannel),
	m_pURB (pURB),
	m_bIn (bIn),
	m_bStatusStage (bStatusStage),
	m_bSplitComplete (FALSE),
	m_nTotalBytesTransfered (0),
	m_nState (0),
	m_nSubState (0),
	m_nTransactionStatus (0),
	m_pTempBuffer (0),
	m_pFrameScheduler (0)
{
	assert (m_pURB != 0);

	m_pEndpoint = pURB->GetEndpoint ();
	assert (m_pEndpoint != 0);
	m_pDevice = m_pEndpoint->GetDevice ();
	assert (m_pDevice != 0);

	m_Speed = m_pDevice->GetSpeed ();
	m_nMaxPacketSize = m_pEndpoint->GetMaxPacketSize ();
	
	m_bSplitTransaction = m_pDevice->IsSplit ();

	if (!bStatusStage)
	{
		if (m_pEndpoint->GetNextPID (bStatusStage) == USBPIDSetup)
		{
			m_pBufferPointer = pURB->GetSetupData ();
			m_nTransferSize = sizeof (TSetupData);
		}
		else
		{
			m_pBufferPointer = pURB->GetBuffer ();
			m_nTransferSize = pURB->GetBufLen ();
		}

		m_nPackets = (m_nTransferSize + m_nMaxPacketSize - 1) / m_nMaxPacketSize;
		
		if (m_bSplitTransaction)
		{
			if (m_nTransferSize > m_nMaxPacketSize)
			{
				m_nBytesPerTransaction = m_nMaxPacketSize;
			}
			else
			{
				m_nBytesPerTransaction = m_nTransferSize;
			}
			
			m_nPacketsPerTransaction = 1;
		}
		else
		{
			m_nBytesPerTransaction = m_nTransferSize;
			m_nPacketsPerTransaction = m_nPackets;
		}
	}
	else
	{
		assert (m_pTempBuffer == 0);
		m_pTempBuffer = new u32[1];
		assert (m_pTempBuffer != 0);
		m_pBufferPointer = m_pTempBuffer;

		m_nTransferSize = 0;
		m_nBytesPerTransaction = 0;
		m_nPackets = 1;
		m_nPacketsPerTransaction = 1;
	}

	assert (m_pBufferPointer != 0);
	assert (((u32) m_pBufferPointer & 3) == 0);

	if (m_bSplitTransaction)
	{
		if (IsPeriodic ())
		{
			m_pFrameScheduler = new CDWHCIFrameSchedulerPeriodic;
		}
		else
		{
			m_pFrameScheduler = new CDWHCIFrameSchedulerNonPeriodic;
		}

		assert (m_pFrameScheduler != 0);
	}
	else
	{
		if (   m_pDevice->GetHubAddress () == 0
		    && m_Speed != USBSpeedHigh)
		{
			m_pFrameScheduler = new CDWHCIFrameSchedulerNoSplit (IsPeriodic ());
			assert (m_pFrameScheduler != 0);
		}
	}
}

CDWHCITransferStageData::~CDWHCITransferStageData (void)
{
	delete m_pFrameScheduler;
	m_pFrameScheduler = 0;

	m_pBufferPointer = 0;

	delete [] m_pTempBuffer;
	m_pTempBuffer = 0;

	m_pEndpoint = 0;
	m_pDevice = 0;
	m_pURB = 0;
}

void CDWHCITransferStageData::TransactionComplete (u32 nStatus, u32 nPacketsLeft, u32 nBytesLeft)
{
#if 0
	if (m_bSplitTransaction)
	{
		CLogger::Get ()->Write ("udata", LogDebug,
					"Transaction complete (status 0x%X, packets 0x%X, bytes 0x%X)",
					nStatus, nPacketsLeft, nBytesLeft);
	}
#endif

	m_nTransactionStatus = nStatus;

	if (  nStatus
	    & (  DWHCI_HOST_CHAN_INT_ERROR_MASK
	       | DWHCI_HOST_CHAN_INT_NAK
	       | DWHCI_HOST_CHAN_INT_NYET))
	{
		return;
	}

	u32 nPacketsTransfered = m_nPacketsPerTransaction - nPacketsLeft;
	u32 nBytesTransfered = m_nBytesPerTransaction - nBytesLeft;

	if (   m_bSplitTransaction
	    && m_bSplitComplete
	    && nBytesTransfered == 0
	    && m_nBytesPerTransaction > 0)
	{
		nBytesTransfered = m_nMaxPacketSize * nPacketsTransfered;
	}
	
	m_nTotalBytesTransfered += nBytesTransfered;
	m_pBufferPointer = (u8 *) m_pBufferPointer + nBytesTransfered;
	
	if (   !m_bSplitTransaction
	    || m_bSplitComplete)
	{
		m_pEndpoint->SkipPID (nPacketsTransfered, m_bStatusStage);
	}

	assert (nPacketsTransfered <= m_nPackets);
	m_nPackets -= nPacketsTransfered;

	// if (m_nTotalBytesTransfered > m_nTransferSize) this will be false:
	if (m_nTransferSize - m_nTotalBytesTransfered < m_nBytesPerTransaction)
	{
		assert (m_nTotalBytesTransfered <= m_nTransferSize);
		m_nBytesPerTransaction = m_nTransferSize - m_nTotalBytesTransfered;
	}
}

void CDWHCITransferStageData::SetSplitComplete (boolean bComplete)
{
	assert (m_bSplitTransaction);
	
	m_bSplitComplete = bComplete;
}

void CDWHCITransferStageData::SetState (unsigned nState)
{
	m_nState = nState;
}

unsigned CDWHCITransferStageData::GetState (void) const
{
	return m_nState;
}

void CDWHCITransferStageData::SetSubState (unsigned nSubState)
{
	m_nSubState = nSubState;
}

unsigned CDWHCITransferStageData::GetSubState (void) const
{
	return m_nSubState;
}

boolean CDWHCITransferStageData::BeginSplitCycle (void)
{
	return TRUE;
}

unsigned CDWHCITransferStageData::GetChannelNumber (void) const
{
	return m_nChannel;
}

boolean CDWHCITransferStageData::IsPeriodic (void) const
{
	assert (m_pEndpoint != 0);
	TEndpointType Type = m_pEndpoint->GetType ();
	
	return    Type == EndpointTypeInterrupt
	       || Type == EndpointTypeIsochronous;
}

u8 CDWHCITransferStageData::GetDeviceAddress (void) const
{
	assert (m_pDevice != 0);
	return m_pDevice->GetAddress ();
}

u8 CDWHCITransferStageData::GetEndpointType (void) const
{
	assert (m_pEndpoint != 0);
	
	unsigned nEndpointType = 0;

	switch (m_pEndpoint->GetType ())
	{
	case EndpointTypeControl:
		nEndpointType = DWHCI_HOST_CHAN_CHARACTER_EP_TYPE_CONTROL;
		break;

	case EndpointTypeBulk:
		nEndpointType = DWHCI_HOST_CHAN_CHARACTER_EP_TYPE_BULK;
		break;

	case EndpointTypeInterrupt:
		nEndpointType = DWHCI_HOST_CHAN_CHARACTER_EP_TYPE_INTERRUPT;
		break;

	default:
		assert (0);
		break;
	}
	
	return nEndpointType;
}

u8 CDWHCITransferStageData::GetEndpointNumber (void) const
{
	assert (m_pEndpoint != 0);
	return m_pEndpoint->GetNumber ();
}

u32 CDWHCITransferStageData::GetMaxPacketSize (void) const
{
	return m_nMaxPacketSize;
}

TUSBSpeed CDWHCITransferStageData::GetSpeed (void) const
{
	return m_Speed;
}

u8 CDWHCITransferStageData::GetPID (void) const
{
	assert (m_pEndpoint != 0);
	
	u8 ucPID = 0;
	
	switch (m_pEndpoint->GetNextPID (m_bStatusStage))
	{
	case USBPIDSetup:
		ucPID = DWHCI_HOST_CHAN_XFER_SIZ_PID_SETUP;
		break;

	case USBPIDData0:
		ucPID = DWHCI_HOST_CHAN_XFER_SIZ_PID_DATA0;
		break;
		
	case USBPIDData1:
		ucPID = DWHCI_HOST_CHAN_XFER_SIZ_PID_DATA1;
		break;

	default:
		assert (0);
		break;
	}
	
	return ucPID;
}

boolean CDWHCITransferStageData::IsDirectionIn (void) const
{
	return m_bIn;
}

boolean CDWHCITransferStageData::IsStatusStage (void) const
{
	return m_bStatusStage;
}

u32 CDWHCITransferStageData::GetDMAAddress (void) const
{
	assert (m_pBufferPointer != 0);

	return (u32) m_pBufferPointer;
}

u32 CDWHCITransferStageData::GetBytesToTransfer (void) const
{
	return m_nBytesPerTransaction;
}

u32 CDWHCITransferStageData::GetPacketsToTransfer (void) const
{
	return m_nPacketsPerTransaction;
}

boolean CDWHCITransferStageData::IsSplit (void) const
{
	return m_bSplitTransaction;
}

boolean CDWHCITransferStageData::IsSplitComplete (void) const
{
	assert (m_bSplitTransaction);
	
	return m_bSplitComplete;
}

u8 CDWHCITransferStageData::GetHubAddress (void) const
{
	assert (m_bSplitTransaction);

	assert (m_pDevice != 0);
	return m_pDevice->GetHubAddress ();
}

u8 CDWHCITransferStageData::GetHubPortAddress (void) const
{
	assert (m_bSplitTransaction);

	assert (m_pDevice != 0);
	return m_pDevice->GetHubPortNumber ();
}

u8 CDWHCITransferStageData::GetSplitPosition (void) const
{
	// only important for isochronous transfers
	return DWHCI_HOST_CHAN_SPLIT_CTRL_ALL;
}

u32 CDWHCITransferStageData::GetStatusMask (void) const
{
	u32 nMask =   DWHCI_HOST_CHAN_INT_XFER_COMPLETE
		    | DWHCI_HOST_CHAN_INT_HALTED
		    | DWHCI_HOST_CHAN_INT_ERROR_MASK;
		    
	if (   m_bSplitTransaction
	    || IsPeriodic ())
	{
		nMask |=   DWHCI_HOST_CHAN_INT_ACK
			 | DWHCI_HOST_CHAN_INT_NAK
			 | DWHCI_HOST_CHAN_INT_NYET;
	}
	
	return	nMask;
}

u32 CDWHCITransferStageData::GetTransactionStatus (void) const
{
	assert (m_nTransactionStatus != 0);
	return m_nTransactionStatus;
}

boolean CDWHCITransferStageData::IsStageComplete (void) const
{
	return m_nPackets == 0;
}

u32 CDWHCITransferStageData::GetResultLen (void) const
{
	if (m_nTotalBytesTransfered > m_nTransferSize)
	{
		return m_nTransferSize;
	}
	
	return m_nTotalBytesTransfered;
}

CUSBRequest *CDWHCITransferStageData::GetURB (void) const
{
	assert (m_pURB != 0);
	return m_pURB;
}

CDWHCIFrameScheduler *CDWHCITransferStageData::GetFrameScheduler (void) const
{
	return m_pFrameScheduler;
}
