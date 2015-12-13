//
// btinquiryresults.cpp
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
#include <circle/bt/btinquiryresults.h>
#include <circle/util.h>
#include <assert.h>

struct TBTInquiryResponse
{
	u8	BDAddress[BT_BD_ADDR_SIZE];
	u8	ClassOfDevice[BT_CLASS_SIZE];
	u8	PageScanRepetitionMode;
	u8	RemoteName[BT_NAME_SIZE];
};

CBTInquiryResults::CBTInquiryResults (void)
{
}

CBTInquiryResults::~CBTInquiryResults (void)
{
	for (unsigned nResponse = 0; nResponse < m_Responses.GetCount (); nResponse++)
	{
		TBTInquiryResponse *pResponse = (TBTInquiryResponse *) m_Responses[nResponse];
		assert (pResponse != 0);

		delete pResponse;
	}
}

void CBTInquiryResults::AddInquiryResult (TBTHCIEventInquiryResult *pEvent)
{
	assert (pEvent != 0);

	for (unsigned i = 0; i < pEvent->NumResponses; i++)
	{
		TBTInquiryResponse *pResponse = new TBTInquiryResponse;
		assert (pResponse != 0);

		memcpy (pResponse->BDAddress, INQUIRY_RESP_BD_ADDR (pEvent, i), BT_BD_ADDR_SIZE);
		memcpy (pResponse->ClassOfDevice, INQUIRY_RESP_CLASS_OF_DEVICE (pEvent, i), BT_CLASS_SIZE);
		pResponse->PageScanRepetitionMode = INQUIRY_RESP_PAGE_SCAN_REP_MODE (pEvent, i);
		strcpy ((char *) pResponse->RemoteName, "Unknown");

		m_Responses.Append (pResponse);
	}
}

boolean CBTInquiryResults::SetRemoteName (TBTHCIEventRemoteNameRequestComplete *pEvent)
{
	assert (pEvent != 0);

	for (unsigned nResponse = 0; nResponse < m_Responses.GetCount (); nResponse++)
	{
		TBTInquiryResponse *pResponse = (TBTInquiryResponse *) m_Responses[nResponse];
		assert (pResponse != 0);

		if (memcmp (pResponse->BDAddress, pEvent->BDAddr, BT_BD_ADDR_SIZE) == 0)
		{
			memcpy (pResponse->RemoteName, pEvent->RemoteName, BT_NAME_SIZE);

			return TRUE;
		}
	}

	return FALSE;
}

unsigned CBTInquiryResults::GetCount (void) const
{
	return m_Responses.GetCount ();
}

const u8 *CBTInquiryResults::GetBDAddress (unsigned nResponse) const
{
	TBTInquiryResponse *pResponse = (TBTInquiryResponse *) m_Responses[nResponse];
	assert (pResponse != 0);

	return pResponse->BDAddress;
}

const u8 *CBTInquiryResults::GetClassOfDevice (unsigned nResponse) const
{
	TBTInquiryResponse *pResponse = (TBTInquiryResponse *) m_Responses[nResponse];
	assert (pResponse != 0);

	return pResponse->ClassOfDevice;
}

const u8 *CBTInquiryResults::GetRemoteName (unsigned nResponse) const
{
	TBTInquiryResponse *pResponse = (TBTInquiryResponse *) m_Responses[nResponse];
	assert (pResponse != 0);

	return pResponse->RemoteName;
}

u8 CBTInquiryResults::GetPageScanRepetitionMode (unsigned nResponse) const
{
	TBTInquiryResponse *pResponse = (TBTInquiryResponse *) m_Responses[nResponse];
	assert (pResponse != 0);

	return pResponse->PageScanRepetitionMode;
}
