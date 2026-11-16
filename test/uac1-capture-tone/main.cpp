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

#ifdef CAPTURE_ADC
#include "adc.h"
#endif
namespace {
#ifdef CAPTURE_ADC
constexpr char Product[] = "Circle ADC Capture";
const TUSBAudioCaptureDescriptors Capture (1, 1, 1, 2, 0x0603);
#else
constexpr char Product[] = "Circle Capture Tone";
const TUSBAudioCaptureDescriptors Capture (1, 1, 1, 2, 0x0603, TUSBAudioCaptureDescriptors::Synchronous);
#endif
const TUSBDeviceDescriptor Device {18,1,0x0200,0,0,0,64,0x1209,0x0001,0x0101,1,2,0,1};
u8 Configuration[100];
void BuildConfiguration (void)
{
 const u8 header[] = {9,2,100,0,2,1,0,0x80,250, 9,4,0,0,0,1,1,0,0, 9,0x24,1,0,1,30,0,1,1};
 u8 *p = Configuration;
 memcpy (p, header, sizeof header); p += sizeof header;
 memcpy (p, Capture.Terminals, sizeof Capture.Terminals); p += sizeof Capture.Terminals;
 memcpy (p, Capture.Streaming, sizeof Capture.Streaming); p += sizeof Capture.Streaming;
 assert (p == Configuration + sizeof Configuration);
}
static_assert (sizeof Configuration == 100);

// One cycle in 48 samples; peak 4096 is -18.06 dBFS.
#ifndef CAPTURE_ADC
const s16 Sine[48] = {0,535,1060,1567,2048,2494,2896,3250,3547,3784,3956,4061,
 4096,4061,3956,3784,3547,3250,2896,2494,2048,1567,1060,535,
 0,-535,-1060,-1567,-2048,-2494,-2896,-3250,-3547,-3784,-3956,-4061,
 -4096,-4061,-3956,-3784,-3547,-3250,-2896,-2494,-2048,-1567,-1060,-535};

class CToneSource : public CUSBAudioCaptureSource
{
public:
 void SetRecording (boolean) override {}
 unsigned ReadPacket (s16 *pSamples) override
 {
  for (unsigned i = 0; i < NominalFrames; ++i)
  {
   *pSamples++ = Sine[i];
   *pSamples++ = Sine[(2*i) % NominalFrames];
  }
  return NominalFrames;
 }
};
#endif
CUSBAudioCaptureSource *Source;
class CToneGadget : public CDWUSBGadget
{
public:
 explicit CToneGadget(CInterruptSystem *irq):CDWUSBGadget(irq,FullSpeed) {}
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
 void AddEndpoints() override { m_EP=new CUSBAudioCaptureEndpoint(Capture,this,Source); }
 void CreateDevice() override {}
 void OnSuspend() override { delete m_EP;m_EP=nullptr; }
 boolean SetInterface(u16 i,u16 alt) override {
  if(i==0)return alt==0;
  return i==1 && alt<=1 && m_EP && m_EP->SetInterface(i,alt);
 }
 int GetInterface(u16 i) override { return i==0?0:i==1 && m_EP?m_EP->GetInterface(i):-1; }
 int OnClassOrVendorRequest(const TSetupData *,u8 *) override {return -1;}
private:
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
#ifdef CAPTURE_ADC
 CADCSource adc(&irq);
 Source=&adc;
#else
 CToneSource tone;
 Source=&tone;
#endif
 CToneGadget gadget(&irq);
 if(!serial.Initialize(115200) || !log.Initialize(&serial) || !irq.Initialize()
    || !timer.Initialize()
#ifdef CAPTURE_ADC
    || !adc.Start()
#endif
    || !gadget.Initialize())halt();
#ifdef CAPTURE_ADC
 unsigned last=timer.GetTicks();
#endif
 while(true) {
  gadget.UpdatePlugAndPlay();
#ifdef CAPTURE_ADC
  unsigned now=timer.GetTicks();
  if(now-last>=5*HZ) { adc.Report(log); last=now; }
#endif
 }
}
