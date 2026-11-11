// SPDX-License-Identifier: GPL-3.0-or-later
//
// wrapper.cpp
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

#include <circle/usb/gadget/usbaudiogadget.h>
#include <circle/usb/gadget/usbaudiogadgetbuffer.h>
#include <cstring>
#include <cstdio>

void WrapperTests (void)
{
 using F = CUSBAudioGadgetFunction;
 F::TConfiguration c {};
 c.nControlInterface=0; c.nStreamingInterface=1; c.nEndpoint=1;
 c.nInputTerminal=1; c.nFeatureUnit=2; c.nOutputTerminal=3;
 c.nChannels=2; c.wChannelConfig=3; c.nAlternates=2;
 c.Alternates[0].Format=F::PCM16;
 c.Alternates[0].nRates=1; c.Alternates[0].Rates[0]=48000;
 c.Alternates[1]=c.Alternates[0]; c.Alternates[1].Format=F::Float32;
 CInterruptSystem irq;
 CUSBAudioGadgetBuffer sink (48000,2,512);
 CUSBAudioGadget g (&irq,c,&sink,0x1234,0x5678);
 size_t length=0;
 auto device=static_cast<const TUSBDeviceDescriptor *>(g.GetDescriptor(0x100,0,&length));
 assert(device && length==18 && device->idVendor==0x1234 && device->idProduct==0x5678);
 auto descriptor=static_cast<const u8 *>(g.GetDescriptor(0x200,0,&length));
 assert(descriptor && descriptor[4]==2 && (descriptor[2]+256U*descriptor[3])==length);
 assert(g.GetDescriptor(0x300,0,&length) && length==4);
 assert(g.GetDescriptor(0x301,0x409,&length) && length==14);
 assert(!g.GetDescriptor(0x303,0x409,&length));
 assert(!g.GetDescriptor(0x600,0,&length));
 assert(g.Initialize());
 auto ep=g.endpoint;
 assert(ep && !ep->active);
 ep->OnUSBReset(); ep->OnActivate();
 assert(!ep->active && ep->arms==0 && g.GetInterface(1)==0);
 assert(g.SetInterface(1,1)); g.Process();
 assert(ep->active && ep->length==192 && g.GetInterface(1)==1);
 memset(ep->buffer,0,192);
 ep->active=false; ep->OnTransferComplete(false,192); g.Process();
 assert(sink.GetBufferedFrames()==48 && ep->arms==2);
 g.OnBusSuspend(); g.Process(); assert(sink.GetBufferedFrames()==0);
 ep->active=false; ep->OnTransferComplete(false,192); g.Process();
 assert(sink.GetBufferedFrames()==0);
 g.OnBusResume(); g.Process();
 ep->active=false; ep->OnTransferComplete(false,192); g.Process();
 assert(sink.GetBufferedFrames()==48);
 assert(g.SetInterface(1,2)); g.Process();
 assert(ep->active && ep->length==384 && ep->cancels==1);
 assert(!g.SetInterface(1,3) && ep->active && ep->cancels==1);
 assert(g.SetInterface(0,0) && ep->active && ep->cancels==1);
 assert(g.SetInterface(1,0)); g.Process();
 assert(!ep->active && g.GetInterface(1)==0);
 ep->OnDeactivate(); g.Process();
 assert(!ep->active && ep->cancels==2 && sink.GetBufferedFrames()==0);
 ep->OnTransferComplete(false,192); assert(!ep->active);
 g.OnSuspend(); assert(!g.endpoint);
 g.AddEndpoints(); g.endpoint->OnUSBReset(); g.endpoint->OnActivate(); g.Process();
 assert(g.GetInterface(1)==0);
 g.endpoint->OnDeactivate(); g.OnSuspend();
 // Two functions may independently allocate interfaces/entities in a composite.
 c.nControlInterface=4; c.nStreamingInterface=5; c.nEndpoint=3;
 c.nInputTerminal=7; c.nFeatureUnit=8; c.nOutputTerminal=9;
 CUSBAudioGadgetBuffer sink2 (48000,2,512);
 F f(c,&sink2); assert(f.IsValid()); f.SetConfigured(true);
 assert(!f.SetInterface(1,1) && f.SetInterface(5,1));
 u8 data[3]{};
 TSetupData request{0xA2,0x81,0x100,1,3}; assert(f.ControlRequest(&request,data)==-1);
 request.wIndex=3; assert(f.ControlRequest(&request,data)==3);
 assert((data[0]+256U*data[1]+65536U*data[2])==48000);
 CUSBAudioGadget invalid (&irq,c,&sink2,0x1234,0x5678);
 assert(!invalid.Initialize()); // Standalone wrapper reserves interfaces 0/1.
 puts("PASS standalone wrapper: descriptors, aligned DMA, lifecycle, controls and composite isolation");
}
