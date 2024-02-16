//
// usbcdcgadgetendpoint.h
//
// This file by Sebastien Nicolas <seba1978@gmx.de>
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
#ifndef _circle_usb_gadget_usbcdcgadgetendpoint_h
#define _circle_usb_gadget_usbcdcgadgetendpoint_h

#include <circle/usb/gadget/dwusbgadgetendpoint.h>
#include <circle/usb/usb.h>
#include <circle/device.h>
#include <circle/synchronize.h>
#include <circle/spinlock.h>
#include <circle/types.h>

class CUSBCDCGadget;
class CUSBSerialDevice;

class CUSBCDCGadgetEndpoint : public CDWUSBGadgetEndpoint /// Endpoint of the USB serial CDC gadget
{
public:
	CUSBCDCGadgetEndpoint (const TUSBEndpointDescriptor *pDesc, CUSBCDCGadget *pGadget);
	~CUSBCDCGadgetEndpoint (void);

	void AttachInterface (CUSBSerialDevice *pInterface);

	void OnActivate (void) override;

	void OnTransferComplete (boolean bIn, size_t nLength) override;

	void OnSuspend (void) override;

private:
	int Write (const void *pData, unsigned nLength);
	int Read (void *pBuffer, unsigned nLength);

	static int WriteHandler (const void *pBuffer, size_t nCount, void *pParam);
	static int ReadHandler (void *pBuffer, size_t nCount, void *pParam);

	unsigned GetQueueBytesFree (void);
	unsigned GetQueueBytesAvail (void);
	void Enqueue (const void *pBuffer, unsigned nCount);
	void Dequeue (void *pBuffer, unsigned nCount);

private:
	CUSBSerialDevice *m_pInterface;

	int m_nStatus;

	volatile boolean m_bInActive;

	static const size_t QueueSize = 8192+1;
	u8 *m_pQueue;
	volatile unsigned m_nInPtr;
	volatile unsigned m_nOutPtr;

	static const size_t MaxOutMessageSize = 512;
	static const size_t MaxInMessageSize = 512;
	DMA_BUFFER (u8, m_OutBuffer, MaxOutMessageSize);
	DMA_BUFFER (u8, m_InBuffer, MaxInMessageSize);

	CSpinLock m_SpinLock;
};

#endif
