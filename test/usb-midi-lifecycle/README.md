# MIDI queue/lifecycle regression tests

Run from the Circle root with Python 3 and a C++17 compiler supporting
AddressSanitizer and UndefinedBehaviorSanitizer:

```sh
python3 test/usb-midi-lifecycle/run.py
python3 test/usb-midi-lifecycle/run.py --source /path/to/other/circle
```

`CXX` selects the compiler. The harness compiles the actual MIDI endpoint source
and header. Stubs replace its base endpoint, MIDI interface, logger, DMA buffer
annotation and spin lock. No hardware is required.

## Failure and fix

The first IN send dequeues its buffer and marks the sender active under the
queue lock, but previously released that lock before calling `BeginTransfer()`.
On a single core, releasing the lock unmasks IRQs. Suspend/deconfiguration/reset
can then clear the queue and retire the endpoint, after which the interrupted
sender starts its stale transfer. The test injects endpoint retirement at that
exact unlock boundary and verifies that no transfer remains armed afterward.
A separate synthetic completion at that boundary checks packet ordering.

The fix starts the first transfer under the existing lock, as the completion
path already does. A configured flag, protected by the same lock, rejects sends
before activation and after suspend/deconfiguration/reset. Reset clears the
MIDI queue before base endpoint reset. Late completions cannot rearm an inactive
endpoint. OUT application callbacks run outside the lock, with configuration
checked again before rearming, allowing callback-triggered deactivation.

Normal activation/reset/deactivation remains serialized by the gadget's IRQ
lifecycle. This does not make arbitrary concurrent endpoint destruction or
reconfiguration safe, nor identify completions from old sessions after a new
activation. The existing synchronous cancellation and interrupt retirement
remain necessary. Queued packets from the old configuration are intentionally
discarded. TRUE from a send means accepted at that time, not delivery guaranteed
across a subsequent reset.

## Coverage and results

Thirteen cases cover inactive send rejection, normal send/drain, reset queue
cleanup, reactivation, retirement at first-send unlock, completion ordering,
late IN/OUT completions, OUT callback deactivation, malformed OUT length,
queue-full rejection and byte-exact FIFO wraparound.

| Source | Passed | Failed |
| --- | ---: | ---: |
| Unmodified develop 4215d68 | 5 | 8 |
| With this patch | 13 | 0 |

ASan/UBSan are enabled. The existing EP0 and OUT-cancellation suites also pass
(21 additional cases), and the ARM32 `RASPPI=3` gadget library builds.

The lock stub checks balanced/nonrecursive acquisition and injects a synchronous
callback when the outer lock is released. The transfer stub records starts,
cancellation and copied bytes. It does not emulate controller registers, DMA,
caches, real interrupt latency or multicore scheduling. These are deterministic
ordering regressions, not a physical USB timing test.

## Earlier hardware evidence and limits

On 2026-10-21, a standalone high-speed MIDI gadget was tested in RAM on a
Raspberry Pi Zero 2 W, ARM32, with a macOS host. During 101 unconfigured telemetry
samples, an interface-reuse-only control accepted 675 more sends and rejected
none. With the combined lifecycle/cancellation candidate, 102 unconfigured
samples accepted no additional sends and rejected 672. Both MIDI directions
resumed after configuration was restored.

The combined candidate also passed CoreMIDI reset recovery: 9600 test CC messages
were received in total, including 3697 in the post-reset window, with no
unexpected-format messages. A direct bulk-transfer fixture passed reset and
deconfiguration/reconfiguration. That fixture used vendor-specific interface
class codes so the host could claim the bulk pipes; MIDI endpoint code and event
format were unchanged.

Those hardware runs included interface reuse and synchronous endpoint
cancellation fixes which have since been upstreamed, along with the local
lifecycle changes. They do not isolate this PR's contribution to reset recovery.
The exact first-send race was forced in software, not at a specific instruction
on hardware. Today's extracted patch adds consistent configured-state locking
and an OUT post-callback recheck; it has been host-tested and ARM-built, not
loaded as a new isolated hardware image. No new hardware validation is claimed
for AArch64, other boards or hosts. This PR does not include composite constructor
generalization, packet-size changes or the separate suspend/resume opt-in API.
