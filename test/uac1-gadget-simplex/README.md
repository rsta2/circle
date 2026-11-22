# UAC1 simplex recording

This test exposes a full-speed USB recording device with two channels of signed
16-bit PCM at 48 kHz. Use a gadget-capable Raspberry Pi (tested on Zero 2 W)
and connect its USB data port to a host with a stereo recorder.

* **Tone (default).** Leave `CAPTURE_ADC` commented out at the top of
  `main.cpp`. No external audio hardware is needed. Select **Circle Capture
  Tone** as the recording input. The left channel carries a 1 kHz sine and the
  right channel a 2 kHz sine, both at about −18 dBFS.
* **ADC.** Uncomment `#define CAPTURE_ADC` and rebuild. Use a stereo I2S ADC
  supplying 48 kHz LRCK and 64 × sample-rate BCK at 3.3 V logic levels. On the
  Zero 2 W connect BCK to GPIO18, LRCK to GPIO19, ADC data out to GPIO20
  (PCM DIN), and share ground. Feed a line-level signal to the ADC and record
  from **Circle ADC Capture**. When you `#define CAPTURE_RPI_SLAVE FALSE`, the
  I2S ADC is clocked from the Raspberry Pi, which is the clocking master then
  (tested with a Waveshare WM8960 HAT).

You have to define your USB vendor ID by adding the following to the
`Config.mk` file (example):

```
DEFINE += -DUSB_GADGET_VENDOR_ID=0x1209
```

With Circle and its libraries built for the target, run
`make -C test/uac1-gadget-simplex` from the Circle root and boot the resulting
kernel image. Select 48 kHz and stereo input 1/2 on the host.
