#!/usr/bin/env python3
"""Exercise the production OUT IRQ dispatcher against a W1C register model."""
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
text = (source / 'lib/usb/gadget/dwusbgadget.cpp').read_text()
start = text.index('void CDWUSBGadget::HandleOutEPInterrupt (void)')
end = text.index('{', start) + 1
depth = 1
while depth:
    depth += (text[end] == '{') - (text[end] == '}')
    end += 1
with tempfile.TemporaryDirectory(prefix='circle-ignored-out-') as temp:
    folder = Path(temp)
    (folder / 'real-gadget.inc').write_text(text[start:end])
    exe = folder / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra',
        '-Werror', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
        '-I' + str(here / 'stubs'), '-I' + str(source / 'include'),
        '-I' + str(folder), str(here / 'test.cpp'), '-o', str(exe)], check=True)
    raise SystemExit(subprocess.run([str(exe)]).returncode)
