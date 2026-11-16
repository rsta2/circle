// SPDX-License-Identifier: GPL-3.0-or-later
#include <circle/startup.h>
#include <circle/koptions.h>
#include <circle/devicenameservice.h>
#include <circle/serial.h>
#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/usb/gadget/dwusbgadget.h>
#include <circle/usb/gadget/usbstringdescriptor.h>
#include <circle/usb/gadget/usbaudiogadgetcapture.h>
#include <circle/util.h>

#include "../uac1-capture-adc/adc.h"
#include <circle/usb/gadget/usbaudiogadgetbuffer.h>
#include <circle/usb/gadget/usbaudiogadget.h>

namespace {
using Function = CUSBAudioGadgetFunction;
Function::TConfiguration PlaybackConfig ()
{
 Function::TConfiguration c {};
 c.nStreamingInterface=2; c.nEndpoint=2;
 c.nInputTerminal=3; c.nFeatureUnit=4; c.nOutputTerminal=5;
 c.nChannels=2; c.wChannelConfig=3; c.nAlternates=1;
 c.Alternates[0]={Function::PCM16,1,{48000}};
 return c;
}
class DuplexI2S : public CADCSource
{
public:
 DuplexI2S(CInterruptSystem *irq, CUSBAudioGadgetBuffer &sink)
 : CADCSource(irq,DeviceModeTXRX), m_Sink(sink) {}
 unsigned GetChunk(u32 *out,unsigned words) override
 {
  s32 samples[128];
  for(unsigned i=0;i<words;) {
   unsigned n=words-i < 128 ? words-i : 128;
   m_Sink.ReadFrames(samples,n/2);
   for(unsigned j=0;j<n;++j) out[i++]=static_cast<u32>(samples[j] >> 8);
  }
  return words;
 }
private:
 CUSBAudioGadgetBuffer &m_Sink;
};
const TUSBEndpointDescriptor OutEndpoint {7,5,2,9,192,1};
const TUSBAudioCaptureDescriptors Capture (1, 1, 1, 2, 0x0603);
const TUSBDeviceDescriptor Device {18,1,0x0200,0,0,0,64,0x1209,0x0001,0x0102,1,2,0,1};
constexpr char Product[]="Circle Duplex Lab";
u8 Configuration[184];
void BuildConfiguration (void)
{
 const u8 header[] = {9,2,184,0,3,1,0,0x80,250, 9,4,0,0,0,1,1,0,0, 10,0x24,1,0,1,62,0,2,1,2};
 const u8 playbackTopology[] = {12,0x24,2,3,1,1,0,2,3,0,0,0, 10,0x24,6,4,3,1,3,0,0,0, 9,0x24,3,5,1,3,0,4,0};
 const u8 playbackStreaming[] = {9,4,2,0,0,1,2,0,0, 9,4,2,1,1,1,2,0,0,
  7,0x24,1,3,1,1,0, 11,0x24,2,1,2,2,16,1,0x80,0xbb,0, 9,5,2,9,192,0,1,0,0, 7,0x25,1,1,0,0,0};
 u8 *p = Configuration;
 memcpy (p, header, sizeof header); p += sizeof header;
 memcpy (p, Capture.Terminals, sizeof Capture.Terminals); p += sizeof Capture.Terminals;
 memcpy (p, playbackTopology, sizeof playbackTopology); p += sizeof playbackTopology;
 memcpy (p, Capture.Streaming, sizeof Capture.Streaming); p += sizeof Capture.Streaming;
 memcpy (p, playbackStreaming, sizeof playbackStreaming); p += sizeof playbackStreaming;
 assert (p == Configuration + sizeof Configuration);
}
static_assert(sizeof Configuration==184);
CADCSource *Source;
class CDuplexGadget : public CDWUSBGadget
{
public:
 explicit CDuplexGadget(CInterruptSystem *irq, Function &play):CDWUSBGadget(irq,FullSpeed),m_Play(play) {}
 const void *GetDescriptor(u16 value,u16 lang,size_t *len) override {
  switch(value) {
  case 0x0100: *len=sizeof Device;return &Device;
  case 0x0200: *len=sizeof Configuration;return Configuration;
  case 0x0300: {static const u8 language[]={4,3,9,4};*len=4;return language;}
  case 0x0301: case 0x0302:
   if(lang==0x409)return USBStringToDescriptor(value==0x0301?"Circle":Product,m_String,sizeof m_String,len);
  }
  return nullptr;
 }
 void AddEndpoints() override { m_EP=new CUSBAudioCaptureEndpoint(Capture,this,Source); m_Out=new CUSBAudioGadgetEndpoint(&OutEndpoint,this,&m_Play); }
 void CreateDevice() override {}
 void OnSuspend() override { delete m_EP;m_EP=nullptr; delete m_Out;m_Out=nullptr; }
 boolean SetInterface(u16 i,u16 alt) override {
  if(i==0)return alt==0;
  if(i==2)return m_Out && m_Out->SetInterface(i,alt);
  return i==1 && alt<=1 && m_EP && m_EP->SetInterface(i,alt);
 }
 int GetInterface(u16 i) override { if(i==2)return m_Play.GetInterface(i); return i==0?0:i==1 && m_EP?m_EP->GetInterface(i):-1; }
 int OnClassOrVendorRequest(const TSetupData *setup,u8 *data) override {return m_Play.ControlRequest(setup,data);}
private:
 Function &m_Play;
 CUSBAudioGadgetEndpoint *m_Out=nullptr;
 CUSBAudioCaptureEndpoint *m_EP=nullptr;
 u8 m_String[64];
};
}

int main()
{
 BuildConfiguration ();
 CKernelOptions options;
 CDeviceNameService devices;
 CSerialDevice serial;
 CExceptionHandler exceptions;
 CInterruptSystem irq;
 CTimer timer(&irq);
 CLogger log(options.GetLogLevel(),&timer);
 CUSBAudioGadgetBuffer sink(48000,2,1024);
 Function play(PlaybackConfig(),&sink);
 DuplexI2S adc(&irq,sink);
 Source=&adc;
 CDuplexGadget gadget(&irq,play);
 if(!serial.Initialize(115200) || !log.Initialize(&serial) || !irq.Initialize()
    || !timer.Initialize()
    || !sink.IsValid() || !play.IsValid() || !adc.Start()
    || !gadget.Initialize())halt();
 unsigned last=timer.GetTicks();
 while(true) {
  gadget.UpdatePlugAndPlay(); play.Process();
  unsigned now=timer.GetTicks();
  if(now-last>=5*HZ) { if(play.GetInterface(2)==0) adc.Report(log); last=now; }
 }
}
