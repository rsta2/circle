# Ignored OUT interrupt acknowledgement

Run from the Circle root with Python 3 and a C++17 compiler supporting ASan/UBSan:

```sh
python3 test/usb-gadget-ignored-out/run.py
python3 test/usb-gadget-ignored-out/run.py --source /path/to/other/circle
```

`CXX` selects the compiler. No hardware is required.

## Failure

`HandleUSBReset()` enables SETUP_DONE, XFER_COMPLETE, EP_DISABLED and AHB_ERROR
in the common OUT interrupt mask. When endpoint dispatch is deliberately skipped
(suspended, or non-control endpoint before ENUMDONE), the old acknowledgement
clears only SETUP_DONE and XFER_COMPLETE. An enabled EP_DISABLED or AHB_ERROR bit
therefore remains pending and can continuously retrigger the OUT interrupt.

The fix writes all ones to the ignored endpoint's W1C interrupt register,
matching the existing ignored IN endpoint policy. Active endpoints still receive
normal dispatch; their status is not cleared by the dispatcher. This retires
ignored status, not the underlying cause of an AHB error, and does not add fault
recovery or diagnostics.

## Model and results

The harness extracts the actual `HandleOutEPInterrupt()` method and includes
Circle's actual register definitions. A register stub models W1C status and
recomputes the endpoint summary from pending enabled causes, so a successful
acknowledgement removes the simulated IRQ source. The enabled causes match the
four flags enabled by the current reset implementation. Endpoint handlers are
counting stubs; device states and endpoint pointers are a small harness fixture.
No controller DMA, cache, bus transactions or physical IRQ timing are simulated.

Twenty cases cover individual and coalesced causes on suspended EP0/non-control
endpoints, non-control endpoints before enumeration, normal active dispatch,
masked endpoints, multiple endpoints in one summary, and a second dispatch after
acknowledgement to check that the ignored status does not cause repeated work.

| Source | Passed | Failed |
| --- | ---: | ---: |
| Unmodified develop 4215d68 | 10 | 10 |
| With this patch | 20 | 0 |

The existing EP0 and OUT-cancellation suites also pass (21 additional tests).
The ARM32 `RASPPI=3` gadget library builds. Sanitizers are enabled for host tests.

## Hardware evidence and limits

The same acknowledgement change was exercised in BioSynth V4.40f on a Raspberry
Pi Zero 2 W, ARM32, with a macOS host on 2026-11-07. RAM testing and a later cold
start retained USB audio, MIDI and ADC operation. A 35-second RAM snapshot showed
zero ADC/USB underruns and overruns, zero DMA resynchronizations and zero I2C
errors. The user reported clean keyboard, pedal, microphone and OLED operation.
A later session snapshot had one USB underrun; its cause was not established.

The instrumentation recorded zero ignored OUT faults during those hardware
checks. Thus hardware testing supports compatibility but does not reproduce the
fault condition or prove it caused any historical freeze. The failing condition
is injected in the host model. The standalone extracted PR was built and tested
on the host, not newly flashed; no new evidence is claimed for other boards,
AArch64 or other hosts. Product-specific counters/emergency reporting are omitted.
