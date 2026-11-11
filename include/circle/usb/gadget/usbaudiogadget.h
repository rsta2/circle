// SPDX-License-Identifier: GPL-3.0-or-later
//
// usbaudiogadget.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
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
#ifndef _circle_usb_gadget_usbaudiogadget_h
#define _circle_usb_gadget_usbaudiogadget_h

#include <circle/usb/gadget/dwusbgadget.h>
#include <circle/usb/gadget/usbaudiogadgetfunction.h>

/// Controller adapter usable by standalone or composite gadgets.
class CUSBAudioGadgetEndpoint : public CDWUSBGadgetEndpoint
{
public:
	CUSBAudioGadgetEndpoint (const TUSBEndpointDescriptor *pDescriptor,
				CDWUSBGadget *pGadget, CUSBAudioGadgetFunction *pFunction);
	void OnUSBReset (void) override;
	void OnActivate (void) override;
	void OnDeactivate (void) override;
	void OnSuspend (void) override;
	void OnTransferComplete (boolean bIn, size_t nLength) override;
private:
	CUSBAudioGadgetFunction *m_pFunction;
	boolean m_bActive;
	unsigned m_nPacketSize;
	DMA_BUFFER (u8, m_Buffer, 1024);
};

/// Standalone UAC1 full-speed adaptive playback gadget.
/// Call UpdatePlugAndPlay() and Process() frequently from the main loop.
/// Configuration/descriptors are immutable after construction.
class CUSBAudioGadget : public CDWUSBGadget
{
public:
	/// Use a vendor/product ID authorized for your device.
	/// Standalone interface IDs must be 0 (AC) and 1 (AS).
	CUSBAudioGadget (CInterruptSystem *pInterruptSystem,
			const CUSBAudioGadgetFunction::TConfiguration &Config,
			CUSBAudioGadgetFunction::CSink *pSink, u16 usVendorID, u16 usProductID);
	~CUSBAudioGadget (void);
	boolean Initialize (boolean bScanDevices = TRUE) override;
	void Process (void);
	void GetStatistics (CUSBAudioGadgetFunction::TStatistics *pStatistics);

	const void *GetDescriptor (u16 wValue, u16 wIndex, size_t *pLength) override;
	boolean SetInterface (u16 nInterface, u16 nAlternate) override;
	int GetInterface (u16 nInterface) override;
	int OnClassOrVendorRequest (const TSetupData *pSetup, u8 *pData) override;
	void AddEndpoints (void) override;
	void CreateDevice (void) override;
	void OnSuspend (void) override;
	boolean RetainTransfersOnSuspend (void) const override;
	void OnBusSuspend (void) override;
	void OnBusResume (void) override;
private:
	CUSBAudioGadgetFunction m_Function;
	CUSBAudioGadgetEndpoint *m_pEndpoint;
	TUSBDeviceDescriptor m_DeviceDescriptor;
	TUSBEndpointDescriptor m_EndpointDescriptor;
	u8 m_ConfigurationDescriptor[CUSBAudioGadgetFunction::MaxDescriptorSize + 9];
	unsigned m_nDescriptorSize;
	u8 m_StringDescriptor[80];
};

#endif
