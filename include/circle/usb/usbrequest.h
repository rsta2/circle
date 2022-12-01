//
// usbrequest.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2022  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_usbrequest_h
#define _circle_usb_usbrequest_h

#include <circle/usb/usb.h>
#include <circle/usb/usbendpoint.h>
#include <circle/classallocator.h>
#include <circle/types.h>

class CUSBRequest;

typedef void TURBCompletionRoutine (CUSBRequest *pURB, void *pParam, void *pContext);

class CUSBRequest		// URB
{
public:
	static const unsigned MaxIsoPackets = 32;

public:
	CUSBRequest (CUSBEndpoint *pEndpoint, void *pBuffer, u32 nBufLen, TSetupData *pSetupData = 0);
	~CUSBRequest (void);

	CUSBEndpoint *GetEndpoint (void) const;

	void SetStatus (int bStatus);
	void SetResultLen (u32 nLength);
	void SetUSBError (TUSBError Error);

	int GetStatus (void) const;
	u32 GetResultLength (void) const;
	TUSBError GetUSBError (void) const;

	TSetupData *GetSetupData (void);
	void *GetBuffer (void);
	u32 GetBufLen (void) const;

	// isochronous transfers are delimited in packets
	void AddIsoPacket (u16 usPacketSize);
	unsigned GetNumIsoPackets (void) const;
	u16 GetIsoPacketSize (unsigned nPacketIndex) const;

	void SetCompletionRoutine (TURBCompletionRoutine *pRoutine, void *pParam, void *pContext);
	void CallCompletionRoutine (void);

	// do not retry if request cannot be served immediately (for Bulk in only)
	void SetCompleteOnNAK (void);
	boolean IsCompleteOnNAK (void) const;
	
private:
	CUSBEndpoint *m_pEndpoint;
	
	TSetupData *m_pSetupData;
	void	   *m_pBuffer;
	u32	    m_nBufLen;
	
	int	    m_bStatus;
	u32	    m_nResultLen;
	TUSBError   m_USBError;

	unsigned    m_nNumIsoPackets;
	u16	    m_usIsoPacketSize[MaxIsoPackets];

	TURBCompletionRoutine *m_pCompletionRoutine;
	void *m_pCompletionParam;
	void *m_pCompletionContext;

	boolean m_bCompleteOnNAK;

	DECLARE_CLASS_ALLOCATOR
};

#endif
