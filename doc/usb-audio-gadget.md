# USB Audio Class 1 playback and recording gadgets

This experimental feature branch provides full-speed UAC1 playback and recording
on the DesignWare USB device controller. Playback supports configurable formats
and rates; recording currently supports 48 kHz stereo signed PCM16. The duplex
test combines both directions with independently controlled USB streams.
The library follows Circle's gadget lifetime and UpdatePlugAndPlay conventions.
Applications supply their audio hardware and routing policy.

## Components

* `CUSBAudioGadgetFunction` generates the audio-function descriptors, validates
  controls, queues incoming packets and applies stream transitions in task context.
* `CUSBAudioGadgetEndpoint` owns the aligned receive DMA buffer and rearms OUT
  transfers. It can be used in a composite gadget.
* `CUSBAudioGadget` is the standalone, two-interface device wrapper.
* `CUSBAudioGadgetBuffer` is an optional software-adaptive sink producing signed
  32-bit interleaved frames for a fixed-rate sound device.
* `TUSBAudioCaptureDescriptors` describes a recording function's terminals and
  streaming interface.
* `CUSBAudioCaptureSource` supplies recording packets, and
  `CUSBAudioCaptureBuffer` adapts an independently clocked PCM16 producer.
* `CUSBAudioCaptureEndpoint` owns the aligned transmit DMA buffer and rearms IN
  transfers.
* `sample/45-usbaudiogadget` demonstrates stereo playback through PWM.
* [Simplex recording](../test/uac1-gadget-simplex/README.md) demonstrates a
  generated stereo tone or an I2S ADC.
* [Duplex audio](../test/uac1-gadget-duplex/README.md) combines I2S playback and
  capture, including independent stream stop/restart.

The playback descriptor topology is USB streaming input terminal -> master
mute/volume feature unit -> speaker output terminal. Alternate zero has no endpoint; each
nonzero alternate advertises one sample format and its discrete rate list.
The audio streaming endpoint is adaptive isochronous OUT, one packet per 1 ms
USB frame. There is no explicit feedback endpoint.

## Playback configuration and limits

Construct a `TConfiguration` and a sink before the gadget. The configuration is
copied and must remain unchanged for the device lifetime. Check `Initialize()`.

Formats are interleaved U8, signed PCM8/16/24/32 and IEEE float32.
Multibyte samples are little-endian. U8 uses the UAC1 PCM8 tag (2) with
silence at 0x80. Signed formats use the PCM tag (1), float32 uses tag 3.
One through eight channels,
up to six distinct formats and up to sixteen sorted, unique rates per format
are supported. All alternates share the channel cluster. A zero channel map
means unspecified channels. Invalid identifiers, duplicate formats/rates and
profiles exceeding the 1023-byte full-speed packet limit are rejected.

The packet ceiling is ceil(rate / 1000) * channels * bytes per sample. Thus not
all combinations of the advertised bounds fit; for example eight channels at
48 kHz/32-bit exceed full-speed capacity. Aggregate bus bandwidth also depends
on other devices and functions and remains the host's scheduling decision.
Descriptors are generated without unaligned stores and without partial output
on insufficient capacity. EP0 has a bounded 1024-byte descriptor buffer.

## Playback control and lifecycle contract

The function supports GET/SET_INTERFACE, endpoint sampling-frequency
GET_CUR/MIN/MAX/RES and SET_CUR, master mute GET_CUR/SET_CUR, and master volume
GET_CUR/MIN/MAX/RES and SET_CUR. Volume is -96 to 0 dB in 1 dB steps, encoded as
signed dB/256. Per-channel controls are not advertised. Invalid direction,
recipient, selector, entity, channel or payload length stalls the control
transfer. Class OUT requests are validated before the status acknowledgement.
Unsupported discrete sampling frequencies round to the nearest available rate,
with ties rounding down, as required by UAC1; GET_CUR returns that rate.

Call both `UpdatePlugAndPlay()` and `Process()` frequently from one task on the
USB core. `Process()` must not run recursively or concurrently from another
core. IRQ operations enqueue packets or mark transitions; sink callbacks run
only from `Process()`, without holding the function lock. This implementation
supports task/IRQ concurrency on one core, not arbitrary multicore callers.

A transition discards queued packets, stops the sink, and configures the latest
stream. Configuration failure leaves playback silent and increments a counter;
a subsequent interface/rate request can retry. A reset during Configure prevents
that old configuration becoming ready. Alternate zero, reset, deconfiguration
and suspend discard pending audio. Retained suspend preserves the selected
format/rate, while resume restarts the sink with fresh packets. Counters are
cumulative and available without logging from interrupt context.

All sink callbacks must finish in bounded time. `Write()` must copy or consume
the supplied samples before returning. `Stop()` must discard downstream audio
and silence the device; it must be safe when already stopped. A USB interrupt
can arrive during a callback: one already executing Write can finish before the
next Process applies Stop. Hardware-specific DMA stop, clock stabilization and
analog mute sequencing belong in the sink. The generic layer cannot guarantee
pop-free analog transitions for an arbitrary codec.

The receive queue holds eight USB packets. Overflow drops the oldest packet to
bound latency. Malformed packet lengths are rejected before decoding. A stalled
application therefore loses audio instead of growing an unbounded backlog.

## Playback clock adaptation

An adaptive endpoint must consume samples at the host's average rate. A fixed
free-running DAC without adaptation will eventually underflow or overflow.
A custom sink can discipline its hardware clock or resample. The optional buffer
uses linear interpolation with a bounded queue-fill correction (+/-2000 ppm)
and nominal-rate conversion, starts after half-buffer prefill, and emits zero
when starved. It clamps float excursions, maps nonfinite floats to silence, and
implements master mute/volume. Overflow discards backlog and prefills again.

This buffer is a reference implementation, not a high-quality anti-aliasing
sample-rate converter. Prefer equal nominal USB/DAC rates for audio-quality
work, or supply a suitably filtered resampler. Its ReadFrames and Write critical
sections are proportional to the requested frame count; use small bounded
chunks as the sample does. The capacity and prefill policy are not a promise of
minimum latency. Neither NEON nor floating-point ABI support is required.

## Playback composite integration

Allocate unique interface numbers, terminal/unit IDs and an unused OUT endpoint.
Append the bytes from GetDescriptor to the composite configuration descriptor,
with its correct total length/interface count and any grouping required by the
composite device's class strategy. Do not include the standalone wrapper's
configuration header. Each function owns its own descriptor configuration and
control state; there is no global audio singleton.

Forward SetInterface to the audio endpoint and GetInterface to its function.
The endpoint cancels and rearms DMA when the streaming alternate changes. Route
class requests by interface/entity or endpoint address to ControlRequest.
Create a CUSBAudioGadgetEndpoint for the function in AddEndpoints, using its
GetMaxPacketSize, isochronous/adaptive attributes (9) and interval 1. Propagate
configured/reset/suspend/resume lifecycle and call Process in the task loop.
The standalone wrapper is the concrete implementation of these steps for one
function. The caller must prevent endpoint/interface collisions with MIDI/CDC.

## Recording configuration and integration

Include `<circle/usb/gadget/usbaudiogadgetcapture.h>`. The capture API is separate
from the configurable playback function and standalone playback wrapper.
It advertises two interleaved signed 16-bit little-endian channels at 48 kHz,
with a left/right channel map. The topology is a physical input terminal feeding
a USB streaming output terminal. Capture has no feature unit, mute/volume
controls or sampling-frequency control. Its fixed rate is in the format
descriptor; alternate zero is idle and alternate one records.

Construct `TUSBAudioCaptureDescriptors` with the streaming interface number, IN
endpoint number (1 through 7, without the direction bit), distinct input/output
terminal IDs, and the physical input terminal type. Reserve these identifiers
within the enclosing gadget. Append `Terminals` to the AudioControl topology and
include their length in its class-specific total length. Add the recording
interface to the AudioControl header's collection, then append `Streaming` after
the AudioControl interface. Adjust the enclosing configuration's total length
and interface count. The test gadgets provide complete descriptor examples.

Create a `CUSBAudioCaptureEndpoint` in the gadget's `AddEndpoints()`, passing the
capture descriptors, gadget and source. Keep the source alive for the endpoint's
lifetime. Forward the capture interface's `SetInterface()` and `GetInterface()`
to this endpoint and propagate the controller's endpoint lifecycle. Continue
calling `UpdatePlugAndPlay()` from the task loop.

The default `Asynchronous` descriptor mode supports an independently clocked
producer. It advertises a maximum packet size of 196 bytes (49 stereo frames)
and an interval of one USB frame (1 ms). `Synchronous` mode advertises 192 bytes
and requires exactly 48 stereo frames per packet; the generated-tone test uses
this mode. Select the mode to match the source's clocking behavior.

## Recording source and lifecycle contract

`CUSBAudioCaptureSource::SetRecording()` and `ReadPacket()` can execute at IRQ
level. They must finish in bounded time without blocking or waiting for hardware.
`ReadPacket()` fills the endpoint's buffer and returns the number of initialized
stereo frames, up to `MaxFrames` (49). The endpoint substitutes 48 frames of
silence for an invalid count, including a count other than 48 in synchronous
mode. It starts the first transfer when recording is enabled and rearms on each
IN completion. Capture has no task-context `Process()` callback.

Alternate zero stops recording and cancels an active transfer. Repeated selection
of the same alternate is idempotent. Deactivation and `OnSuspend()` stop the source
and clear configuration state. USB reset also stops an active source and resets
endpoint transfer state. A nonzero alternate is accepted only after activation.
Applications using retained suspend must explicitly handle its lifecycle; capture
does not provide the playback function's retained-format resume policy.

The optional `CUSBAudioCaptureBuffer` accepts stereo PCM16 through
`WriteFrames()`, discarding input while recording is inactive. Its 1024-frame
ring primes at 256 frames. On priming, excess backlog is discarded so a delayed
first host IN request or a recording restart starts from the reserve level.
Every eighth packet, queue occupancy can adjust its length to 47 or 49 frames,
matching gradual producer/USB clock drift without interpolation. Use this buffer
with asynchronous descriptors.

Overflow discards the oldest frames. Starvation emits silence and requires
prefill again. `WriteFrames()` bounds each lock-held block to 64 frames.
`GetStatistics()` reports produced, sent, queued, overrun and underrun counts;
the counters reset when recording is enabled. Produced/sent/overrun counts use
frames, while underruns count transitions from a primed queue to starvation.

## Duplex operation

The duplex test combines a playback function/endpoint and a capture source/IN
endpoint under one AudioControl interface. Both test streams use 48 kHz stereo
PCM16. Each USB streaming interface has its own alternate setting, so either
stream can stop and restart while the other continues.

The test keeps the duplex I2S hardware running across USB stream changes.
Playback stop silences its sink; recording stop discards capture input. Shared
I2S clocks and DMA therefore continue serving the active direction. Applications
that reconfigure shared hardware must coordinate both directions themselves.

## Validation and remaining hardware work

Host tests compile the production function, adaptive buffer and controller
sources with narrow platform/MMIO substitutes, warnings-as-errors and address/
undefined-behavior sanitizers. They cover 384 format/channel/rate profiles,
100,000 deterministic malformed-control/lifecycle interleavings, bounded queues,
clock-drift simulations, EP0 request/status ordering and packet termination,
isochronous parity/cancellation/rearm, and the existing MIDI/suspend regressions.
Capture tests also cover asynchronous/synchronous descriptors, delayed first IN
requests, restart priming, stereo continuity, clock corrections, starvation and
endpoint activation, cancellation, suspend and reset. Run
`python3 unittest/usb-audio-gadget/run.py` from the repository root.
The PWM sample cross-builds and links for AArch32 RASPPI=3.

The [QEMU harness](../test/usb-audio-qemu/README.md) boots real Circle ARM
code and tests the production function and adaptive buffer with 18,000 simulated
USB packets across six formats and three input rates. It checks rendered audio
numerically and exports a WAV preview, with optional playback on macOS. QEMU
does not emulate the Raspberry Pi USB gadget controller or PWM audio, so this
is software-path validation, not enumeration, DMA or real-time validation.

For hardware validation, exercise play/stop, alternate changes and suspend/resume
on the target host. PWM output fidelity and long-term clock drift need separate
measurements. The QEMU checks do not replace USB compliance testing.

Further work includes configurable recording formats/rates/channel counts,
capture feature-unit controls, UAC2, high-speed audio, explicit feedback endpoints,
hardware-specific clock switching, per-channel volume and USB compliance
certification. The QEMU harness described above exercises playback; capture and
duplex hardware checks use the linked test programs.

Specification: [USB Device Class Definition for Audio Devices, Release 1.0](https://www.usb.org/sites/default/files/audio10.pdf).
