#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compare current buffer with a specified Git revision on the host CPU."""
import argparse
import csv
import io
import json
import os
from pathlib import Path
import platform
import statistics
import subprocess
import tempfile

here = Path(__file__).resolve().parent
root = here.parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--baseline', required=True)
parser.add_argument('--output', required=True, type=Path)
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
compiler = os.environ.get('CXX', 'c++')
revision = subprocess.check_output(['git', 'rev-parse', '--verify', args.baseline+'^{commit}'], cwd=root, text=True).strip()
baseline = subprocess.check_output(['git', 'show', revision+':lib/usb/gadget/usbaudiogadgetbuffer.cpp'], cwd=root)
with tempfile.TemporaryDirectory(prefix='circle-buffer-benchmark-') as directory:
    temp = Path(directory)
    old = temp/'reference.cpp'
    old.write_bytes(b'#define CUSBAudioGadgetBuffer CReferenceAudioBuffer\n'+baseline)
    common = [compiler, '-std=c++14', '-Wall', '-Wextra', '-Werror',
              '-I', str(root/'unittest/usb-audio-gadget/stubs'), '-I', str(root/'include'),
              str(here/'main.cpp'), str(old),
              str(root/'lib/usb/gadget/usbaudiogadgetbuffer.cpp'),
              str(root/'lib/usb/gadget/usbaudiogadgetfunction.cpp')]
    regression = temp/'regression'
    subprocess.run(common+['-O1', '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-o', str(regression)], check=True)
    result = subprocess.check_output([str(regression)], text=True)
    (args.output/'regression.log').write_text(result)
    print(result, end='')
    benchmark = temp/'benchmark'
    subprocess.run(common+['-O2', '-o', str(benchmark)], check=True)
    results = subprocess.check_output([str(benchmark), '--timing'], text=True)
    (args.output/'timings.csv').write_text(results)
rows = list(csv.DictReader(io.StringIO(results)))
summary = []
for key in dict.fromkeys((r['format'], r['channels'], r['operation']) for r in rows):
    group = [r for r in rows if (r['format'], r['channels'], r['operation']) == key]
    before = statistics.median(float(r['baseline_ns']) for r in group)
    after = statistics.median(float(r['optimized_ns']) for r in group)
    summary.append(dict(format=key[0], channels=int(key[1]), operation=key[2],
                        baseline_ns=before, optimized_ns=after, speedup=before/after))
report = dict(scope='Host CPU, platform spinlock substitutes; not a Zero timing measurement',
              machine=platform.platform(), baseline=revision,
              compiler=subprocess.check_output([compiler, '--version'], text=True).splitlines()[0],
              flags='-O2; no LTO; 9 alternating-order rounds; 100000 packets per round', results=summary)
(args.output/'summary.json').write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps(report, indent=2))
