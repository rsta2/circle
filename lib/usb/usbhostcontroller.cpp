//
// usbhostcontroller.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2016  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/usbhostcontroller.h>
#include <circle/timer.h>
#include <assert.h>

CUSBHostController::CUSBHostController (void)
{
}

CUSBHostController::~CUSBHostController (void)
{
}
	
int CUSBHostController::GetDescriptor (CUSBEndpoint	*pEndpoint, 
				       unsigned char	 ucType,
				       unsigned char	 ucIndex,
				       void		*pBuffer,
				       unsigned		 nBufSize,
				       unsigned char	 ucRequestType,
				       unsigned short	 wIndex)
{
	return ControlMessage (pEndpoint,
			       ucRequestType, GET_DESCRIPTOR,
			       (ucType << 8) | ucIndex, wIndex,
			       pBuffer, nBufSize);
}

boolean CUSBHostController::SetAddress (CUSBEndpoint *pEndpoint, u8 ucDeviceAddress)
{
	if (ControlMessage (pEndpoint, REQUEST_OUT, SET_ADDRESS, ucDeviceAddress, 0, 0, 0) < 0)
	{
		return FALSE;
	}
	
	CTimer::Get ()->MsDelay (50);		// see USB 2.0 spec (tDSETADDR)
	
	return TRUE;
}

boolean CUSBHostController::SetConfiguration (CUSBEndpoint *pEndpoint, u8 ucConfigurationValue)
{
	if (ControlMessage (pEndpoint, REQUEST_OUT, SET_CONFIGURATION, ucConfigurationValue, 0, 0, 0) < 0)
	{
		return FALSE;
	}
	
	CTimer::Get ()->MsDelay (50);
	
	return TRUE;
}

int CUSBHostController::ControlMessage (CUSBEndpoint *pEndpoint,
					u8 ucRequestType, u8 ucRequest,
					u16 usValue, u16 usIndex,
					void *pData, u16 usDataSize)
{
	TSetupData *pSetup = new TSetupData;
	assert (pSetup != 0);

	pSetup->bmRequestType = ucRequestType;
	pSetup->bRequest      = ucRequest;
	pSetup->wValue	      = usValue;
	pSetup->wIndex	      = usIndex;
	pSetup->wLength	      = usDataSize;

	CUSBRequest URB (pEndpoint, pData, usDataSize, pSetup);

	int nResult = -1;

	if (SubmitBlockingRequest (&URB))
	{
		nResult = URB.GetResultLength ();
	}
	
	delete pSetup;

	return nResult;
}

int CUSBHostController::Transfer (CUSBEndpoint *pEndpoint, void *pBuffer, unsigned nBufSize)
{
	CUSBRequest URB (pEndpoint, pBuffer, nBufSize);

	if (!SubmitBlockingRequest (&URB))
	{
		return -1;
	}

	return URB.GetResultLength ();
}
