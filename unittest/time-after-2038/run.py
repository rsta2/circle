#!/usr/bin/env python3
"""Run the 2038 and NTP era regression against the real CTimer and CNTPClient code."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

here = Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source', type=Path, default=here.parents[1])
args = parser.parse_args()


def between(text, first, end):
    """The text from the line that starts with first up to the line that starts with end."""
    start = text.index('\n' + first) + 1
    return text[start:text.index('\n' + end, start) + 1]


def without_includes(path):
    """The file's text without its #include lines, whose names test.cpp stands in for."""
    text = (args.source / path).read_text()
    return '\n'.join(line for line in text.splitlines() if not line.startswith('#include')) + '\n'


timer = (args.source / 'lib/timer.cpp').read_text()
with tempfile.TemporaryDirectory() as tmp:
    folder = Path(tmp)
    (folder / 'timer.inc').write_text(
        between(timer, 'boolean CTimer::SetTimeZone (', 'unsigned CTimer::GetClockTicks (void)')
        + between(timer, 'unsigned CTimer::GetTicks (void) const', 'CString *CTimer::GetTimeString (void)'))
    (folder / 'ntpclient.h.inc').write_text(without_includes('include/circle/net/ntpclient.h'))
    (folder / 'ntpclient.inc').write_text(without_includes('lib/net/ntpclient.cpp'))
    exe = folder / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-I', tmp, str(here / 'test.cpp'), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
