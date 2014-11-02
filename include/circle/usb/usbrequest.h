//
// usbrequest.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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
#ifndef _usbrequest_h
#define _usbrequest_h

#include <circle/usb/usb.h>
#include <circle/usb/usbendpoint.h>
#include <circle/types.h>

class CUSBRequest;

typedef void TURBCompletionRoutine (CUSBRequest *pURB, void *pParam, void *pContext);

class CUSBRequest		// URB
{
public:
	CUSBRequest (CUSBEndpoint *pEndpoint, void *pBuffer, u32 nBufLen, TSetupData *pSetupData = 0);
	~CUSBRequest (void);

	CUSBEndpoint *GetEndpoint (void) const;

	void SetStatus (int bStatus);
	void SetResultLen (u32 nLength);

	int GetStatus (void) const;
	u32 GetResultLength (void) const;

	TSetupData *GetSetupData (void);
	void *GetBuffer (void);
	u32 GetBufLen (void) const;
	
	void SetCompletionRoutine (TURBCompletionRoutine *pRoutine, void *pParam, void *pContext);
	void CallCompletionRoutine (void);
	
private:
	CUSBEndpoint *m_pEndpoint;
	
	TSetupData *m_pSetupData;
	void	   *m_pBuffer;
	u32	    m_nBufLen;
	
	int	    m_bStatus;
	u32	    m_nResultLen;
	
	TURBCompletionRoutine *m_pCompletionRoutine;
	void *m_pCompletionParam;
	void *m_pCompletionContext;
};

#endif
