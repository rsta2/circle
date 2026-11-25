#!/usr/bin/env python3
"""Test PL011 startup and receive callback ordering with the driver source."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

here = Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source', type=Path, default=here.parents[1])
args = parser.parse_args()
driver = (args.source / 'lib/serial.cpp').read_text()
start = driver.index('boolean CSerialDevice::Initialize (')
end = driver.index('\nint CSerialDevice::Write (', start)
setter = driver.index('void CSerialDevice::RegisterCharReceivedHandler (')
setter_end = driver.index('void CSerialDevice::RegisterMagicReceivedHandler (', setter)
irq = driver.index('void CSerialDevice::InterruptHandler (')
irq_end = driver.index('void CSerialDevice::InterruptStub (', irq)
prefixes = ('INT_', 'ARM_UART_', 'CR_', 'IFLS_', 'LCRH_', 'DR_', 'FR_')
defines = [line for line in driver.splitlines() if line.lstrip().startswith('#define ')
           and line.lstrip().split()[1].startswith(prefixes)]
with tempfile.TemporaryDirectory() as tmp:
    folder = Path(tmp)
    (folder / 'pl011.inc').write_text('\n'.join(defines))
    (folder / 'initialize.inc').write_text(driver[start:end])
    (folder / 'callback.inc').write_text(driver[setter:setter_end] + driver[irq:irq_end])
    for source, model in [('test.cpp', 3), ('test.cpp', 5), ('callback.cpp', 3)]:
        exe = folder / 'test'
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra',
                        '-Werror', f'-DRASPPI={model}', '-I', tmp, str(here / source),
                        '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
