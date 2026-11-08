# Bulk OUT cancellation regression

Run from the Circle root with Python 3 and a C++17 compiler supporting
AddressSanitizer and UndefinedBehaviorSanitizer:

```sh
python3 test/usb-gadget-out-cancel/run.py
```

`CXX` selects the compiler. `--source /path/to/circle` tests another checkout
with the same harness. No Raspberry Pi or USB device is needed.

The test compiles the actual `dwusbgadgetendpoint.cpp` and extracts the actual
`CDWUSBGadget::WaitForBit()` and `FlushTxFIFO()` implementations. MMIO, time,
cache operations and application callbacks are stubbed. Panic/assertion handling
throws so the harness can inspect state after a fatal path. This does not alter
production panic handling.

The model checks buffer ownership while endpoint disable is pending, W1C
interrupt acknowledgement, EPENA preservation in the EPDIS command, and global
NAK ownership. It does not establish physical USB timing, cache coherency, or
behavior on other controller revisions.

## Results

Tested on macOS with Clang and ASan/UBSan against `develop` ddab4c9:

| Source | Passed | Failed |
| --- | ---: | ---: |
| Unmodified develop | 3 | 3 |
| With this fix | 6 | 0 |

Cases cover missing and normal global OUT NAK acknowledgement, failed disable
with stale EP_DISABLED status, completion during the missing-NAK wait,
pre-existing global NAK ownership, and an already stopped endpoint. In the
failure case, NAK and disable waits remain bounded at 10 ms each; the software
transfer and buffer remain owned when disable fails. ARM32 `RASPPI=3` gadget
library build also passed.

## Hardware evidence (2026-11-07)

Platform: Raspberry Pi Zero 2 W (BCM2837), ARM32 Circle application, macOS USB
host, composite audio/MIDI/CDC gadget. UART and an ESP32-S3 JTAG probe captured
the failure before recovery.

During powered USB detach/reattach after an unresponsive USB session, reset
from suspended state reached this fatal cancellation path:

```text
usb-test: USBRETRY: data reattached
...
dwgadget: USB suspend (state 1)
dwgadget: USB reset (state 1)
EMERGENCY kind=00000001 time=3055E762 USB=84009438 mask=000E3800
PANIC log tail:
dwgadget: USB reset (state 1)
dwgadget: Timeout
dwhci: Register at 0x14 is 0x84009438
dwgadgetep: EP1 OUT NAK timeout
```

With the warning/disable fallback in BioSynth V4.43k, four powered reconnects
completed and returned diagnostic reports. For example, reconnect 4 recorded:

```text
dwgadget: USB suspend (state 4)
dwgadget: USB reset (state 1)
dwgadget: Enumeration done at Full-Speed (state 2)
...
dwgadget: 6 Endpoint(s) activated
audio-usb: RECONFIGURED: existing MIDI interface reused
usb-test: MIDI interface CONFIGURED
```

These successful reconnects do not establish that the rare missing-NAK condition
occurred again on hardware. The injected MMIO regression exercises that path,
including disable failure. Hardware runs used the composite application's local
Circle tree, including an EP0 recovery change. This standalone patch was built
and host-tested separately against current upstream; it has not been reflashed
as an isolated Circle sample. Deliberate reconnect interrupts playback and must
not be assessed as a steady-state audio interval.
