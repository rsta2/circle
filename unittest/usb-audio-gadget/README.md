# UAC1 playback function and adaptive buffer

Run `python3 unittest/usb-audio-gadget/run.py` from the repository root.
The test builds the actual function and buffer sources using a host C++ compiler,
ASan/UBSan and warnings-as-errors. Platform stubs model single-core critical
sections and reject application callbacks invoked under the function lock.

Coverage includes descriptor lengths and topology across 320 profiles, rejected
configurations, buffer boundaries, exact control recipient/selector/length
validation, deferred transitions, configure failure, reset during callbacks,
suspend/resume, alternate zero, queue overflow, invalid packet lengths, and
100,000 deterministic randomized request/lifecycle operations. Buffer tests cover
sample conversion extremes, float nonfinite values, mute/volume, stop/restart,
120-second clock-drift simulations at -800/0/+800 ppm, and nominal conversion
from 32/44.1/48 kHz to 48 kHz.

The generated traffic models application/control boundaries, not malformed USB
wire packets or host-controller behavior. Real host enumeration and DWC DMA
hardware must be tested separately before merging the feature.

The generated AudioControl descriptors are also parsed by the unmodified
production `CUSBAudioFunctionTopology` host parser. Tests check terminal types,
channel counts, source links, upstream traversal, and master-only mute/volume.
Only diagnostic logging/string formatting is substituted on the host; parsing
and entity/control interpretation use Circle's actual implementation.

The shared gadget string converter is tested for exact UTF-16LE output, empty
strings, exact-fit and maximum-length buffers, rejected non-ASCII input,
insufficient space and null arguments. Invalid inputs must leave the destination
unchanged and return a zero length (when a length pointer is supplied).

Buffer regressions also cover unaligned PCM32 input, signed full-scale endpoints,
zero/full/oversized writes, and repeated wraparound in a 17-frame ring for 1–8
channels. The optional [baseline comparison](../../test/usb-audio-buffer-benchmark/README.md)
adds sanitized differential tests and host CPU timing.
