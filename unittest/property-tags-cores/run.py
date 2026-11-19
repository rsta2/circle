#!/usr/bin/env python3
"""Run the property tags regression against the real CBcmPropertyTags, with threads as cores."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

here = Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source', type=Path, default=here.parents[1])
args = parser.parse_args()


def without_includes(path):
    """The file's text without its #include lines, whose names test.cpp stands in for."""
    text = (args.source / path).read_text()
    return '\n'.join(line for line in text.splitlines() if not line.startswith('#include')) + '\n'


with tempfile.TemporaryDirectory() as tmp:
    folder = Path(tmp)
    (folder / 'header.inc').write_text(without_includes('include/circle/bcmpropertytags.h'))
    (folder / 'source.inc').write_text(without_includes('lib/bcmpropertytags.cpp'))
    exe = folder / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-pthread', '-I', tmp, str(here / 'test.cpp'), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True, timeout=60)
