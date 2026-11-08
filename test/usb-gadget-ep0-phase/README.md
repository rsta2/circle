# EP0 SETUP phase recovery regression

Run from the Circle root with Python 3 and a C++17 compiler supporting
AddressSanitizer and UndefinedBehaviorSanitizer:

```sh
python3 test/usb-gadget-ep0-phase/run.py
```

`CXX` selects the compiler. `--source /path/to/circle` tests another checkout
with this same harness. No Raspberry Pi or USB device is needed.

The test compiles the actual `dwusbgadgetendpoint.cpp` and
`dwusbgadgetendpoint0.cpp`. It also extracts and compiles the actual
`CDWUSBGadget::WaitForBit()` and `FlushTxFIFO()` implementations. Stubs replace
MMIO, time, cache operations and the application callbacks; they do not replace
the endpoint state machines. Panic/assertion handling throws to let the harness
inspect the state at failure. Production panic handling remains unchanged.

The MMIO model provides W1C status, delayed IN NAK/disable acknowledgement,
FIFO flush completion and injected timeouts. It checks that EP0 OUT never
receives EPDIS and that IN ownership is not retired before stopping hardware.
It cannot establish physical USB timing, cache coherency or behavior on other
controller revisions.

## Results

Tested on macOS with Clang and ASan/UBSan against `develop` ddab4c9:

| Source | Passed | Failed |
| --- | ---: | ---: |
| Unmodified develop | 5 | 10 |
| With this fix | 15 | 0 |

Cases cover normal IN/OUT control phases, coalesced SETUP/XFRC, residuals
120/127/1, SETUP arriving during each previous control phase or no software
transfer, a single validated residual snapshot, and fatal NAK/disable/flush
timeouts without premature retirement. ARM32 `RASPPI=3` gadget library build
also passed. The tests require no changes to Circle's target build.

## Hardware evidence (2026-11-07)

Platform: Raspberry Pi Zero 2 W (BCM2837), ARM32 Circle application, macOS USB
host, composite audio/MIDI/CDC gadget. UART and an ESP32-S3 JTAG probe were used
before recovery. Addresses below refer to the corresponding BioSynth builds,
not to an upstream Circle sample.

First frozen startup, BioSynth V4.43h:

```text
PC 0x25428: assertion_failed
emergency record: assertion, line 438, uptime 6.843058 seconds
FinishTransfer(): nXferSize <= m_nTransferLength
software phase TransferSetupOut, armed length 8
DOEPTSIZ0 = 0x40080078; remaining byte count = 120
```

A subsequent startup with only a SETUP-phase residual guard still failed:

```text
V4.43k, tenth startup
PC 0x25448: assertion_failed, line 441
saved return 0x46d80: SETUP_DONE -> FinishTransfer()
saved r5 = 1: TransferDataOut
OUT residual = 120
```

The second capture establishes that SETUP completion can be handled with the
previous OUT transfer's software record. It does not identify its higher-level
control phase. A separate endpoint-object memory dump had unverified cache
visibility and was not used as authoritative evidence.

After the phase/residual recovery and single-snapshot change, V4.43l completed
10 RAM startups in 451 seconds, with 25 diagnostic snapshots, 10 rate exercises
and return to 48 kHz, and three powered USB detach/reattach cycles. All ten
pre-restart snapshots had USB playing, zero USB/ADC under/overruns, zero DMA
resync/late/unknown events and zero I2C errors. Deliberate detach transitions
can increment underrun counters; these are distinct from steady-state checks.
The user confirmed keyboard, pedal, microphone/reverb and OLED operation with
clean audio afterward.

These hardware runs exercised this recovery logic in the composite application
with other local changes present. This standalone PR was separately built and
host-tested against current upstream; it has not been reflashed as an isolated
Circle sample. Successful runs do not prove every intermittent freeze is fixed
or establish why the controller produced the unexpected residual.
