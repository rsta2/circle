#!/usr/bin/env python3
"""Host MMIO regression tests against real Circle endpoint sources.

Pass --source /path/to/unmodified/circle to demonstrate the pre-fix failures.
Requires Python 3 and a C++17 compiler with Address/UndefinedBehaviorSanitizer.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

here = Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source', type=Path, default=here.parents[1])
args = parser.parse_args()
source = args.source.resolve()
gadget = (source / 'lib/usb/gadget/dwusbgadget.cpp').read_text()
helpers = []
for marker in ('int CDWUSBGadget::GetInterface (',
               'boolean CDWUSBGadget::SetInterface (',
               'boolean CDWUSBGadget::FlushTxFIFO (',
               'boolean CDWUSBGadget::WaitForBit ('):
    start = gadget.index(marker)
    end = gadget.index('{', start) + 1
    depth = 1
    while depth:
        depth += (gadget[end] == '{') - (gadget[end] == '}')
        end += 1
    helpers.append(gadget[start:end])
with tempfile.TemporaryDirectory(prefix='circle-ep0-phase-') as tmp:
    folder = Path(tmp)
    (folder / 'real-gadget.inc').write_text('\n\n'.join(helpers))
    binary = folder / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra',
                    '-Werror', '-Wno-unused-parameter', '-g',
                    '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                    '-I', str(here / 'stubs'), '-I', str(source / 'include'),
                    '-I', str(folder), str(here / 'test.cpp'),
                    str(source / 'lib/usb/gadget/dwusbgadgetendpoint.cpp'),
                    str(source / 'lib/usb/gadget/dwusbgadgetendpoint0.cpp'),
                    '-o', str(binary)], check=True)
    raise SystemExit(subprocess.run([str(binary)]).returncode)
