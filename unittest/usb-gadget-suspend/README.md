# Opt-in USB bus suspend/resume

A gadget can opt in by overriding `RetainTransfersOnSuspend()` to return TRUE.
The default is FALSE: existing gadgets still suspend their endpoints and wait
for a USB reset, as before. This does not change the meaning of the existing
application `OnSuspend()` teardown callback.

For an opted-in gadget, bus suspend retains configuration, endpoint transfer
ownership, data toggles and the EP0 control phase. WKUP restores the previous
device state without recreating interfaces or restarting armed transfers.
`OnBusSuspend()` and `OnBusResume()` run at IRQ level. Application work must be
bounded and nonblocking: gate new traffic on suspend, preserve DMA buffers,
and resume application scheduling on wake. Do not reconfigure/delete endpoints
in these callbacks. Pending endpoint completions still dispatch while suspended
so completed transfers are not left owned indefinitely. Application completion
callbacks must respect the suspended state when deciding whether to queue more
traffic. `IsBusSuspended()` reports retained suspend, not the legacy policy.

A USB reset clears retained suspend and follows normal endpoint retirement and
re-enumeration. It does not call `OnBusResume()`. A gadget must therefore handle
its reset/reconfiguration path as well as wake. For coalesced lifecycle IRQs,
reset takes precedence over wake/suspend; otherwise wake takes precedence over
stale suspend. Repeated suspend/wake notifications are idempotent.

This is bus lifecycle support, not remote-wakeup signaling, VBUS/disconnect
detection, CPU power management or certification of suspend-current limits.
No existing gadget is opted in by this patch.

## Regression tests

From the Circle root, with Python 3 and a C++17 compiler supporting ASan/UBSan:

```sh
python3 unittest/usb-gadget-suspend/run.py
```

`CXX` selects the compiler. `--source /path/to/circle` runs the same harness
against another source tree. No hardware is required.

The harness compiles the actual endpoint and EP0 source files, plus the actual
interrupt enable/dispatch, suspend/resume, reset, endpoint dispatch and polling
methods extracted from `dwusbgadget.cpp`. Stubs provide MMIO, time, cache
operations and application callbacks. Device enumeration and configuration
callbacks are simplified; this is not an end-to-end host enumeration test.
For the pre-fix baseline only, the absent resume method is represented by an
empty harness adapter so direct-call cases link. The actual old IRQ handler
and interrupt mask are used unchanged.

The MMIO model checks W1C status and delayed endpoint stop/FIFO flush behavior.
It cannot validate physical USB timing or cache coherency. The fifteen cases
cover interrupt unmasking, all resumable device states, four EP0 control phases,
reset/wake precedence, IN and OUT completion dispatch while suspended, actual
EP0 IN ownership retirement, legacy behavior and unsolicited wake.

Results on macOS/Clang with ASan and UBSan:

| Source | Passed | Failed |
| --- | ---: | ---: |
| Unmodified develop 4215d68 | 2 | 13 |
| With this patch | 15 | 0 |

The existing EP0 phase and OUT cancellation suites also pass (21 additional
cases). The ARM32 `RASPPI=3` gadget library builds with the actual public header
and existing gadget subclasses, which retain the default policy.

## Hardware evidence (2026-11-08)

Platform: Raspberry Pi Zero 2 W (BCM2837), ARM32 Circle, macOS host and BioSynth
composite audio/MIDI/CDC gadget. UART and an ESP32-S3 JTAG probe were available.

After a soak continuation lost USB access, the capture showed the CPU in its
normal main loop, gadget state `StateSuspended`, and WKUP pending but masked.
Mac power logs showed idle sleep at 09:01:32 followed by dark-wake/sleep cycles
and user wake at 09:58:29. This evidence identified a missing USB resume path;
it was not a whole-kernel freeze.

The opt-in implementation was then tested in BioSynth V4.43v in RAM. The user
put the Mac to sleep for about 20 seconds and woke it without changing cables.
The OLED stayed powered, uptime continued and the user confirmed that audio,
keyboard and pedal returned. Diagnostic excerpts after wake:

```text
uptime=175s ... ADC-rate=48000 expected=48000 clocks=OK
ADC queue=16 under=0 over=0 | USB playing=1 queue=72 under=2 over=0
DMA TX/RX resync=0/0 late=0/0 unknown=0/0
USB bus suspended=0 suspends=2 resumes=2
```

MIDI session was ready and I2C errors remained zero. USB underruns increased
from 0 to 2 across the transition, then stayed at 2 in three subsequent
snapshots spanning 40 seconds; ADC under/over remained zero. This is evidence
of recovery, not uninterrupted playback while the host sleeps.

The hardware run used the composite application's local Circle tree and
application-specific traffic gating, with other local changes present. This
standalone PR was independently built and host-tested against current upstream;
it was not reflashed as an isolated Circle sample. Other Raspberry Pi models,
AArch64, other hosts and suspend-current compliance have not been hardware-tested.
The evidence covers the observed sequence, not every suspend/resume scenario.
