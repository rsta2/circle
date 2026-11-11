// SPDX-License-Identifier: GPL-3.0-or-later
//
// dwusbgadget.h
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

#pragma once
#include <circle/usb/usb.h>
#include <cassert>
#define DMA_BUFFER(type, name, count) alignas(64) type name[count]
class CInterruptSystem {};
class CDWUSBGadgetEndpoint;
class CDWUSBGadget
{
public:
 enum TDeviceSpeed { FullSpeed };
 CDWUSBGadgetEndpoint *endpoint = nullptr;
 CDWUSBGadget (CInterruptSystem *, TDeviceSpeed) {}
 virtual ~CDWUSBGadget () {}
 virtual boolean Initialize (boolean = TRUE) { AddEndpoints (); return TRUE; }
 virtual const void *GetDescriptor (u16, u16, size_t *) = 0;
 virtual boolean SetInterface (u16, u16) = 0;
 virtual int GetInterface (u16) = 0;
 virtual int OnClassOrVendorRequest (const TSetupData *, u8 *) = 0;
 virtual void AddEndpoints () = 0;
 virtual void CreateDevice () = 0;
 virtual void OnSuspend () = 0;
 virtual boolean RetainTransfersOnSuspend () const = 0;
 virtual void OnBusSuspend () = 0;
 virtual void OnBusResume () = 0;
};
// This substitute checks the wrapper's ownership and lifecycle. The separate
// MMIO tests compile the actual DWC endpoint cancellation and frame logic.
class CDWUSBGadgetEndpoint
{
public:
 enum TTransferMode { TransferDataOut };
 bool active = false;
 unsigned arms = 0, cancels = 0;
 void *buffer = nullptr;
 size_t length = 0;
 CDWUSBGadget *owner;
 CDWUSBGadgetEndpoint (const TUSBEndpointDescriptor *, CDWUSBGadget *g) : owner(g)
 { assert (!g->endpoint); g->endpoint = this; }
 virtual ~CDWUSBGadgetEndpoint () { assert (!active); owner->endpoint = nullptr; }
 virtual void OnUSBReset () { active = false; }
 virtual void OnActivate () = 0;
 virtual void OnDeactivate () = 0;
 virtual void OnSuspend () = 0;
 virtual void OnTransferComplete (boolean, size_t) = 0;
 bool IsTransferActive () const { return active; }
 void BeginTransfer (TTransferMode, void *p, size_t n)
 { assert (!active); assert (!(reinterpret_cast<uintptr_t>(p) % 64)); active=true; buffer=p; length=n; ++arms; }
 void CancelTransfer () { assert (active); active=false; ++cancels; }
};
