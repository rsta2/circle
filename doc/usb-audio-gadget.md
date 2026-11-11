# USB Audio Class 1 playback gadget

This experimental feature branch provides full-speed UAC1 **playback** on the
DesignWare USB device controller. It is independent of any synthesizer, codec,
I2S clock generator, display or MIDI routing policy. It follows Circle's gadget
lifetime and UpdatePlugAndPlay conventions.

## Components

* `CUSBAudioGadgetFunction` generates the audio-function descriptors, validates
  controls, queues incoming packets and applies stream transitions in task context.
* `CUSBAudioGadgetEndpoint` owns the aligned receive DMA buffer and rearms OUT
  transfers. It can be used in a composite gadget.
* `CUSBAudioGadget` is the standalone, two-interface device wrapper.
* `CUSBAudioGadgetBuffer` is an optional software-adaptive sink producing signed
  32-bit interleaved frames for a fixed-rate sound device.
* `sample/45-usbaudiogadget` demonstrates stereo playback through PWM.

The descriptor topology is USB streaming input terminal -> master mute/volume
feature unit -> speaker output terminal. Alternate zero has no endpoint; each
nonzero alternate advertises one sample format and its discrete rate list.
The audio streaming endpoint is adaptive isochronous OUT, one packet per 1 ms
USB frame. There is no explicit feedback endpoint.

## Configuration and limits

Construct a `TConfiguration` and a sink before the gadget. The configuration is
copied and must remain unchanged for the device lifetime. Check `Initialize()`.

Formats are signed PCM8, PCM16, packed PCM24, PCM32, and IEEE float32, all
little-endian, interleaved. PCM8 here is signed PCM (format tag 1), not the
unsigned PCM8 format tag. Float32 uses format tag 3. One through eight channels,
up to five distinct formats and up to sixteen sorted, unique rates per format
are supported. All alternates share the channel cluster. A zero channel map
means unspecified channels. Invalid identifiers, duplicate formats/rates and
profiles exceeding the 1023-byte full-speed packet limit are rejected.

The packet ceiling is ceil(rate / 1000) * channels * bytes per sample. Thus not
all combinations of the advertised bounds fit; for example eight channels at
48 kHz/32-bit exceed full-speed capacity. Aggregate bus bandwidth also depends
on other devices and functions and remains the host's scheduling decision.
Descriptors are generated without unaligned stores and without partial output
on insufficient capacity. EP0 has a bounded 1024-byte descriptor buffer.

## Control and lifecycle contract

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

## Clock adaptation

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

## Composite integration

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

## Validation and remaining hardware work

Host tests compile the production function, adaptive buffer and controller
sources with narrow platform/MMIO substitutes, warnings-as-errors and address/
undefined-behavior sanitizers. They cover 320 format/channel/rate profiles,
100,000 deterministic malformed-control/lifecycle interleavings, bounded queues,
clock-drift simulations, EP0 request/status ordering and packet termination,
isochronous parity/cancellation/rearm, and the existing MIDI/suspend regressions.
The PWM sample cross-builds and links for AArch32 RASPPI=3.

The [QEMU harness](../test/usb-audio-qemu/README.md) boots real Circle ARM
code and tests the production function and adaptive buffer with 15,000 simulated
USB packets across five formats and three input rates. It checks rendered audio
numerically and exports a WAV preview, with optional playback on macOS. QEMU
does not emulate the Raspberry Pi USB gadget controller or PWM audio, so this
is software-path validation, not enumeration, DMA or real-time validation.

For hardware validation, exercise play/stop, alternate changes and suspend/resume
on the target host. PWM output fidelity and long-term clock drift need separate
measurements. The QEMU checks do not replace USB compliance testing.

Not implemented: capture/isochronous IN, full duplex, UAC2, high-speed audio,
explicit feedback endpoints, hardware-specific clock switching, per-channel
volume or USB compliance certification. Those require separate design/review;
this branch is a complete playback path, not the entire UAC specification.

Specification: [USB Device Class Definition for Audio Devices, Release 1.0](https://www.usb.org/sites/default/files/audio10.pdf).
