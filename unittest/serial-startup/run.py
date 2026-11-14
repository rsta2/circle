#!/usr/bin/env python3
"""Run the PL011 startup ordering regression against the real initializer."""
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
defines = [line for line in driver.splitlines() if line.lstrip().startswith('#define ')
           and line.lstrip().split()[1].startswith(('INT_', 'ARM_UART_', 'CR_', 'IFLS_', 'LCRH_'))]
with tempfile.TemporaryDirectory() as tmp:
    folder = Path(tmp)
    (folder / 'pl011.inc').write_text('\n'.join(defines))
    (folder / 'initialize.inc').write_text(driver[start:end])
    for model in (3, 5):
        exe = folder / 'test'
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra',
                        '-Werror', f'-DRASPPI={model}', '-I', tmp, str(here / 'test.cpp'),
                        '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
