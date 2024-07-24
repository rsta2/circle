//
// usbmstgadgetendpoint.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2023-2024  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_gadget_usbmsdgadgetendpoint_h
#define _circle_usb_gadget_usbmsdgadgetendpoint_h

#include <circle/usb/gadget/dwusbgadgetendpoint.h>
#include <circle/usb/usb.h>
#include <circle/device.h>
#include <circle/synchronize.h>
#include <circle/spinlock.h>
#include <circle/types.h>

class CUSBMSDGadget;


class CUSBMSDGadgetEndpoint : public CDWUSBGadgetEndpoint /// Endpoint of the USB mass storage gadget
{
public:
	CUSBMSDGadgetEndpoint (const TUSBEndpointDescriptor *pDesc, CUSBMSDGadget *pGadget);
	~CUSBMSDGadgetEndpoint (void);

	void OnActivate (void) override;

	void OnTransferComplete (boolean bIn, size_t nLength) override;

	void OnSuspend (void) override;

	enum TMSDTransferMode //since TTransferMode is protected, can't use in the gadget class
	{
		TransferCBWOut,	
		TransferDataOut,
		TransferDataIn,
		TransferCSWIn

	};	

	void BeginTransfer (TMSDTransferMode Mode, void *pBuffer, size_t nLength);

	void StallRequest(boolean bIn);

private:
    CUSBMSDGadget *m_pGadget;

	static const size_t MaxOutMessageSize = 512;
	static const size_t MaxInMessageSize = 512;
	DMA_BUFFER (u8, m_OutBuffer, MaxOutMessageSize);
	DMA_BUFFER (u8, m_InBuffer, MaxInMessageSize);

	CSpinLock m_SpinLock;
};

#endif
