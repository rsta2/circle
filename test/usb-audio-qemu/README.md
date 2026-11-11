# QEMU ARM audio-processing test

This boots real Circle ARM code on QEMU's `raspi0` machine and links the production
`CUSBAudioGadgetFunction` and `CUSBAudioGadgetBuffer`. No platform or spinlock stubs
are used. A temporary source copy keeps the normal hardware build intact.

Stock QEMU's Raspberry Pi model lacks DWC2 USB **device** mode and PWM audio.
Consequently the harness supplies simulated 1 ms USB packets directly to the
function and exports rendered samples through ARM semihosting. It does not
emulate a USB audio device visible to macOS. The standalone gadget wrapper,
physical endpoint DMA, host enumeration and real interrupt timing are not tested
here. `usb-audio` in QEMU is a host-side emulated peripheral, not a replacement
for the gadget controller.

Requirements: Python 3, GNU Make, `arm-none-eabi-` toolchain and `qemu-system-arm`.
From the Circle root:

```sh
python3 test/usb-audio-qemu/run.py --output /tmp/circle-uac1-audio-results
```

The output contains the ARM image, build/guest logs, input packets, signed 32-bit
rendered samples, numerical analysis, source hashes and a stereo WAV preview.
The test covers five formats (signed PCM8/16/24/32 and float32) at 32/44.1/48 kHz,
15,000 deliberately unaligned packets, transitions, mute, suspend/resume and
alternate zero. It checks
post-prefill continuity, counters, output RMS/peak, 440/660 Hz pitch within 1 Hz,
and more than 35 dB separation between the channel test tones.

For a short audible demonstration on macOS, add `--play`. This compiles the
optional CoreAudio player and selects `BuiltInSpeakerDevice` explicitly. It does
not change the system's default audio device, sample rate or speaker volume.
The queue volume is 50%, with source tones peaking at 8% and faded boundaries.
The 4.25-second preview plays left, right, then both for each of the five formats.
Audio service access may require running outside an application sandbox.

Semihosting gives the emulated guest access to host files. Run only this trusted
image; the supplied harness reads input.bin and writes rendered.s32 in the chosen
output directory. This is offline emulation, not a real-time performance or
latency measurement. Listening is useful but does not replace hardware tests.

[QEMU Raspberry Pi device coverage](https://www.qemu.org/docs/master/system/arm/raspi.html)
