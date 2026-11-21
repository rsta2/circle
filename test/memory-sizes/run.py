#!/usr/bin/env python3
"""Host test of the sizes CMemorySystem in lib/memory64.cpp takes from the RAM size.

Pass --source /path/to/unmodified/circle to see the failures without the fix.
Requires Python 3 and a C++17 compiler with AddressSanitizer and
UndefinedBehaviorSanitizer.
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

# Destructor () and EnableMMU () read and write Arm system registers, so
# test.cpp has its own. The other asm statements run only with the MMU enabled.
REPLACED = ('void CMemorySystem::Destructor (void)', 'void CMemorySystem::EnableMMU (void)')


def without(text, marker):
    """TEXT without the function whose definition starts with MARKER."""
    start = text.find(marker + '\n{')
    if start < 0:
        raise SystemExit(f'lib/memory64.cpp: {marker} not found')
    end = text.index('{', start) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[:start] + text[end:]


memory = (source / 'lib/memory64.cpp').read_text()
for marker in REPLACED:
    memory = without(memory, marker)
memory = '#define TEST_ASM(...) ((void) 0)\n' + memory.replace('asm volatile', 'TEST_ASM')

# A Raspberry Pi 5, and with KASAN_SUPPORTED a Raspberry Pi 4 and a Raspberry Pi 5.
BUILDS = {
    'aarch64-rpi5': ['-DRASPPI=5'],
    'aarch64-rpi4-kasan': ['-DRASPPI=4', '-DKASAN_SUPPORTED'],
    'aarch64-rpi5-kasan': ['-DRASPPI=5', '-DKASAN_SUPPORTED'],
}

with tempfile.TemporaryDirectory(prefix='circle-memory-sizes-') as tmp:
    folder = Path(tmp)
    (folder / 'memory64.cpp').write_text(memory)
    failed = []
    for name, defines in BUILDS.items():
        binary = folder / f'test-{name}'
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra',
                        '-Werror', '-Wno-unused-parameter', '-g',
                        '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                        '-DAARCH=64', *defines,
                        '-I', str(here / 'stubs'), '-I', str(source / 'include'),
                        str(here / 'test.cpp'), str(folder / 'memory64.cpp'),
                        '-o', str(binary)], check=True)
        print(f'{name}:', flush=True)
        if subprocess.run([str(binary)]).returncode != 0:
            failed.append(name)
    if failed:
        raise SystemExit('failed: ' + ', '.join(failed))
