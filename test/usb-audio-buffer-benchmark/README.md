# Adaptive audio buffer comparison

Run from the Circle root:

```sh
python3 test/usb-audio-buffer-benchmark/run.py --baseline cca5cb2 --output /tmp/circle-buffer-results
```

The baseline must contain the pre-optimization buffer and have a compatible
public header. It is read from Git without switching branches. Both versions
compile in a temporary directory, with the baseline class renamed so the two
can run in one process. No hardware, firmware or host audio settings change.

First, ASan/UBSan differential tests compare 320 scenarios with 1000 operations
each: 1–8 channels, all five formats, four ring capacities including non-powers
of two, input rates above/below the output rate, random full-range samples,
unaligned input, zero/full/oversized writes, overflow, starvation, mute, volume
and stop/reconfigure. Samples, return values, queue occupancy and drift
correction must match exactly.

Timing uses `-O2` without sanitizers or LTO, nine alternating-order rounds and
100,000 packets per round. CSV results contain nanoseconds per packet; JSON
reports medians and compiler/platform identity. Cases use stereo 48-frame or
8-channel 24-frame packets and a 257-frame ring. Write-only cases include the
normal overflow/discard/refill behaviour; write+read cases exercise ongoing
playback with queue-fill adaptation. Both versions must produce equal checksums.

These are **host CPU** timings with substituted platform locks and the host's
`memcpy`, not Raspberry Pi latency or cycle measurements. QEMU separately checks
ARM functional behaviour; target CPU performance still needs hardware measurement.
