//
// btlogicallayer.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015  R. Stange <rsta2@o2online.de>
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
#include <circle/bt/btlogicallayer.h>
#include <circle/bt/bluetooth.h>
#include <circle/util.h>
#include <assert.h>

CBTLogicalLayer::CBTLogicalLayer (CBTHCILayer *pHCILayer)
:	m_pHCILayer (pHCILayer),
	m_pInquiryResults (0),
	m_pBuffer (0)
{
}

CBTLogicalLayer::~CBTLogicalLayer (void)
{
	assert (m_pInquiryResults == 0);

	delete [] m_pBuffer;
	m_pBuffer = 0;

	m_pHCILayer = 0;
}

boolean CBTLogicalLayer::Initialize (void)
{
	m_pBuffer = new u8[BT_MAX_DATA_SIZE];
	assert (m_pBuffer != 0);

	return TRUE;
}

void CBTLogicalLayer::Process (void)
{
	assert (m_pHCILayer != 0);
	assert (m_pBuffer != 0);

	unsigned nLength;
	while (m_pHCILayer->ReceiveLinkEvent (m_pBuffer, &nLength))
	{
		assert (nLength >= sizeof (TBTHCIEventHeader));
		TBTHCIEventHeader *pHeader = (TBTHCIEventHeader *) m_pBuffer;

		switch (pHeader->EventCode)
		{
		case EVENT_CODE_INQUIRY_RESULT: {
			assert (nLength >= sizeof (TBTHCIEventInquiryResult));
			TBTHCIEventInquiryResult *pEvent = (TBTHCIEventInquiryResult *) pHeader;
			assert (nLength >=   sizeof (TBTHCIEventInquiryResult)
					   + pEvent->NumResponses * INQUIRY_RESP_SIZE);

			assert (m_pInquiryResults != 0);
			m_pInquiryResults->AddInquiryResult (pEvent);
			} break;

		case EVENT_CODE_INQUIRY_COMPLETE: {
			assert (nLength >= sizeof (TBTHCIEventInquiryComplete));
			TBTHCIEventInquiryComplete *pEvent = (TBTHCIEventInquiryComplete *) pHeader;

			if (pEvent->Status != BT_STATUS_SUCCESS)
			{
				delete m_pInquiryResults;
				m_pInquiryResults = 0;

				m_Event.Set ();

				break;
			}

			m_nNameRequestsPending = m_pInquiryResults->GetCount ();
			if (m_nNameRequestsPending == 0)
			{
				m_Event.Set ();

				break;
			}

			assert (m_pInquiryResults != 0);
			for (unsigned nResponse = 0; nResponse < m_pInquiryResults->GetCount (); nResponse++)
			{
				TBTHCIRemoteNameRequestCommand Cmd;
				Cmd.Header.OpCode = OP_CODE_REMOTE_NAME_REQUEST;
				Cmd.Header.ParameterTotalLength = PARM_TOTAL_LEN (Cmd);
				memcpy (Cmd.BDAddr, m_pInquiryResults->GetBDAddress (nResponse), BT_BD_ADDR_SIZE);
				Cmd.PageScanRepetitionMode = m_pInquiryResults->GetPageScanRepetitionMode (nResponse);
				Cmd.Reserved = 0;
				Cmd.ClockOffset = CLOCK_OFFSET_INVALID;
				m_pHCILayer->SendCommand (&Cmd, sizeof Cmd);
			}
			} break;

		case EVENT_CODE_REMOTE_NAME_REQUEST_COMPLETE: {
			assert (nLength >= sizeof (TBTHCIEventRemoteNameRequestComplete));
			TBTHCIEventRemoteNameRequestComplete *pEvent = (TBTHCIEventRemoteNameRequestComplete *) pHeader;

			if (pEvent->Status == BT_STATUS_SUCCESS)
			{
				assert (m_pInquiryResults != 0);
				m_pInquiryResults->SetRemoteName (pEvent);
			}

			if (--m_nNameRequestsPending == 0)
			{
				m_Event.Set ();
			}
			} break;

		default:
			break;
		}
	}
}

CBTInquiryResults *CBTLogicalLayer::Inquiry (unsigned nSeconds)
{
	assert (1 <= nSeconds && nSeconds <= 61);
	assert (m_pHCILayer != 0);

	assert (m_pInquiryResults == 0);
	m_pInquiryResults = new CBTInquiryResults;
	assert (m_pInquiryResults != 0);

	m_Event.Clear ();

	TBTHCIInquiryCommand Cmd;
	Cmd.Header.OpCode = OP_CODE_INQUIRY;
	Cmd.Header.ParameterTotalLength = PARM_TOTAL_LEN (Cmd);
	Cmd.LAP[0] = INQUIRY_LAP_GIAC       & 0xFF;
	Cmd.LAP[1] = INQUIRY_LAP_GIAC >> 8  & 0xFF;
	Cmd.LAP[2] = INQUIRY_LAP_GIAC >> 16 & 0xFF;
	Cmd.InquiryLength = INQUIRY_LENGTH (nSeconds);
	Cmd.NumResponses = INQUIRY_NUM_RESPONSES_UNLIMITED;
	m_pHCILayer->SendCommand (&Cmd, sizeof Cmd);

	m_Event.Wait ();

	CBTInquiryResults *pResult = m_pInquiryResults;
	m_pInquiryResults = 0;
	
	return pResult;
}
