#!/usr/bin/env python3
"""Run the counter to clock ticks regression against the real timer header."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

here = Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source', type=Path, default=here.parents[1])
args = parser.parse_args()
header = (args.source / 'include/circle/timer.h').read_text()
defines = [line for line in header.splitlines() if line.startswith('#define CLOCKHZ')]
start = header.index('static u64 CounterToClockTicks (')
end = header.index('\n\t}\n', start) + len('\n\t}\n')
with tempfile.TemporaryDirectory() as tmp:
    folder = Path(tmp)
    (folder / 'defines.inc').write_text('\n'.join(defines) + '\n')
    (folder / 'function.inc').write_text(header[start:end])
    exe = folder / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-I', tmp, str(here / 'test.cpp'), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
