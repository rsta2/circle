//
// btinquiryresults.h
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
#ifndef _circle_bt_btinquiryresults_h
#define _circle_bt_btinquiryresults_h

#include <circle/bt/bluetooth.h>
#include <circle/ptrarray.h>
#include <circle/types.h>

class CBTInquiryResults
{
public:
	CBTInquiryResults (void);
	~CBTInquiryResults (void);

	void AddInquiryResult (TBTHCIEventInquiryResult *pEvent);
	boolean SetRemoteName (TBTHCIEventRemoteNameRequestComplete *pEvent);

	unsigned GetCount (void) const;

	const u8 *GetBDAddress (unsigned nResponse) const;
	const u8 *GetClassOfDevice (unsigned nResponse) const;
	const u8 *GetRemoteName (unsigned nResponse) const;
	u8 GetPageScanRepetitionMode (unsigned nResponse) const;

private:
	CPtrArray m_Responses;
};

#endif
