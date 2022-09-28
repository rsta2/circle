//
// usbhiddevice.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_usbhiddevice_h
#define _circle_usb_usbhiddevice_h

#include <circle/usb/usbfunction.h>
#include <circle/usb/usbendpoint.h>
#include <circle/usb/usbrequest.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/types.h>

class CUSBHIDDevice : public CUSBFunction
{
public:
	// nMaxReportSize can be handed-over here or to Configure()
	CUSBHIDDevice (CUSBFunction *pFunction, unsigned nMaxReportSize = 0);
	~CUSBHIDDevice (void);

	boolean ConfigureHID (unsigned nMaxReportSize = 0);

protected:
	// has to be called from Configure() in derived class, when initialization is done
	boolean StartRequest (void);

	// is called, when report arrives
	virtual void ReportHandler (const u8 *pReport,
				    unsigned nReportSize) = 0;		// pReport is 0 on failure

	// cannot be called from ReportHandler() context
	boolean SendToEndpointOut (const void *pBuffer, unsigned nBufSize,
				   unsigned nTimeoutMs = USB_TIMEOUT_NONE);
	// send is not completed on return (asynchronous operation)
	boolean SendToEndpointOutAsync (const void *pBuffer, unsigned nBufSize,
					unsigned nTimeoutMs = USB_TIMEOUT_NONE);
	// returns resulting length or < 0 on failure, must not be used after StartRequest()
	int ReceiveFromEndpointIn (void *pBuffer, unsigned nBufSize,
				   unsigned nTimeoutMs = USB_TIMEOUT_NONE);

private:
	void CompletionRoutine (CUSBRequest *pURB);
	static void CompletionStub (CUSBRequest *pURB, void *pParam, void *pContext);

	static void SendCompletionRoutine (CUSBRequest *pURB, void *pParam, void *pContext);

private:
	unsigned m_nMaxReportSize;

	CUSBEndpoint *m_pReportEndpoint;
	CUSBEndpoint *m_pEndpointOut;		// interrupt out EP (optional)

	u8 *m_pReportBuffer;
};

#endif
