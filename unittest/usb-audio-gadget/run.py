#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compile production audio-function source with host-only platform substitutes."""
from pathlib import Path
import os
import subprocess
import tempfile
here = Path(__file__).resolve().parent
root = here.parents[1]
with tempfile.TemporaryDirectory(prefix='circle-uac1-') as tmp:
    binary = Path(tmp) / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++14', '-Wall', '-Wextra',
                    '-Werror', '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                    '-I', str(here/'stubs'), '-I', str(root/'include'),
                    str(here/'test.cpp'), str(here/'strings.cpp'),
                    str(root/'lib/usb/gadget/usbstringdescriptor.cpp'),
                    str(root/'lib/usb/usbaudiofunctopology.cpp'), str(root/'lib/usb/gadget/usbaudiogadgetfunction.cpp'),
                    str(root/'lib/usb/gadget/usbaudiogadgetbuffer.cpp'),
                    str(root/'lib/usb/gadget/usbaudiogadget.cpp'), str(here/'wrapper.cpp'),
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
