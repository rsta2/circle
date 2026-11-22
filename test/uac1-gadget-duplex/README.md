# UAC1 duplex playback and recording

This test exposes **Circle Duplex Lab** with independent USB playback and
recording streams. Both use stereo signed 16-bit PCM at 48 kHz. With Circle
and its libraries built for the target, run `make -C test/uac1-duplex` from
the Circle root and boot the resulting kernel image.

You have to define your USB vendor ID by adding the following to the
`Config.mk` file (example):

```
DEFINE += -DUSB_GADGET_VENDOR_ID=0x1209
```

Use a gadget-capable Raspberry Pi (tested on Zero 2 W), a USB data cable to
the host, a stereo I2S ADC and a stereo I2S DAC. Configure the ADC to supply
48 kHz LRCK and 64 × sample-rate BCK. The Pi and DAC follow those clocks.
On the Zero 2 W use 3.3 V logic and a common ground. Connect BCK to GPIO18,
LRCK to GPIO19, ADC data out to GPIO20 (PCM DIN), and GPIO21 (PCM DOUT)
to the DAC data input. When you `#define CAPTURE_RPI_SLAVE FALSE` in 
`main.cpp`, the I2S ADC is clocked from the Raspberry Pi, which is the 
clocking master then (tested with a Waveshare WM8960 HAT).

For a loopback test, connect the DAC's stereo line output to the ADC's line
input. On the host select Circle Duplex Lab for input and output at 48 kHz
and turn off input monitoring to avoid feedback. Play different tones on
left and right. Keep recording while stopping and restarting playback, then
keep playback running while closing and reopening recording. The recording
should contain both tones on their respective channels during playback and
quiet intervals when playback stops.
