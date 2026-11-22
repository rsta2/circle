#!/usr/bin/env python3
"""Compile the actual MIDI endpoint implementation and header with host stubs."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source', type=Path)
args = parser.parse_args()
here = Path(__file__).resolve().parent
source = args.source.resolve() if args.source else here.parents[1]
with tempfile.TemporaryDirectory(prefix='circle-midi-') as temp:
    exe = Path(temp) / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra',
        '-Werror', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
        '-I' + str(here / 'stubs'), '-I' + str(source / 'include'),
        str(here / 'test.cpp'), str(source / 'lib/usb/gadget/usbmidigadgetendpoint.cpp'),
        '-o', str(exe)], check=True)
    raise SystemExit(subprocess.run([str(exe)]).returncode)
