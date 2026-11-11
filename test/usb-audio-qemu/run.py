#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run production UAC1 processing on emulated ARM; no USB gadget/PWM emulation."""
from pathlib import Path
import argparse
import array
import hashlib
import json
import math
import shutil
import struct
import subprocess
import sys
import tempfile
import wave

here = Path(__file__).resolve().parent
root = here.parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--play', action='store_true', help='macOS built-in speakers only')
args = parser.parse_args()
p = args.output.resolve()
p.mkdir(parents=True, exist_ok=True)
for program in ('arm-none-eabi-g++', 'qemu-system-arm', 'make'):
    if not shutil.which(program):
        raise SystemExit('Missing tool: ' + program)

with (p/'input.bin').open('wb') as f:
    for fmt in range(5):
        for rate in (32000, 44100, 48000):
            for i in range(rate):
                env = min(1, i/(rate*.03), (rate-1-i)/(rate*.03))
                for hz in (440, 660):
                    v = .08*env*math.sin(2*math.pi*hz*i/rate)
                    if fmt == 4:
                        f.write(struct.pack('<f', v))
                    else:
                        size = fmt+1
                        q = round(v*((1 << (size*8-1))-1))
                        f.write(q.to_bytes(size, 'little', signed=True))

with tempfile.TemporaryDirectory(prefix='circle-uac1-qemu-') as temp:
    build = Path(temp)
    # Copy tracked source files, including current edits, but not existing build
    # products or Config.mk. The hardware build configuration stays untouched.
    files = subprocess.check_output(['git', 'ls-files', '-z'], cwd=root).decode().split('\0')
    for name in files:
        source = root/name
        if not name or not source.is_file():
            continue
        destination = build/name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)
    shutil.copytree(here, build/'test/usb-audio-qemu', dirs_exist_ok=True)
    with (p/'build.log').open('w') as log:
        subprocess.run(['bash', './configure', '-r', '1', '--qemu', '-d', 'LEAVE_QEMU_ON_HALT'],
                       cwd=build, stdout=log, stderr=subprocess.STDOUT, check=True)
        for directory in ('lib', 'addon/qemu', 'lib/usb/gadget', 'test/usb-audio-qemu'):
            subprocess.run(['make', '-C', directory, '-j4'], cwd=build,
                           stdout=log, stderr=subprocess.STDOUT, check=True)
    image = build/'test/usb-audio-qemu/kernel.img'
    shutil.copy2(image, p/'kernel.img')
    cmd = ['qemu-system-arm', '-M', 'raspi0', '-bios', str(image),
           '-display', 'none', '-serial', 'null', '-monitor', 'none',
           '-semihosting-config', 'enable=on,target=native']
    with (p/'qemu.log').open('w') as log:
        subprocess.run(cmd, cwd=p, stdout=log, stderr=subprocess.STDOUT,
                       timeout=60, check=True)
    log = (p/'qemu.log').read_text()
    if 'PASS 15000 packets' not in log or 'FAIL' in log:
        raise SystemExit('QEMU guest did not complete its checks: ' + log)
    print(log)

a=array.array('i');a.frombytes((p/'rendered.s32').read_bytes())
if sys.byteorder!='little':a.byteswap()
assert len(a)==15*48000*2
report=[]
for fmt in range(5):
 for ri,rate in enumerate((32000,44100,48000)):
  case=fmt*3+ri
  channels=[]
  for ch,hz in enumerate((440,660)):
   data=[a[case*96000+2*i+ch]/2147483648 for i in range(7200,40800)]
   rms=math.sqrt(sum(v*v for v in data)/len(data));peak=max(abs(v) for v in data)
   crossings=[i+(0-data[i])/(data[i+1]-data[i]) for i in range(len(data)-1) if data[i]<=0<data[i+1]]
   freq=(len(crossings)-1)*48000/(crossings[-1]-crossings[0])
   def energy(f):
    z=sum(v*complex(math.cos(2*math.pi*f*i/48000),-math.sin(2*math.pi*f*i/48000)) for i,v in enumerate(data));return abs(z)
   sep=20*math.log10(max(energy(hz),1e-12)/max(energy(660 if ch==0 else 440),1e-12))
   assert .05<rms<.063 and peak<.09,(fmt,rate,ch,rms,peak)
   assert abs(freq-hz)<1,(fmt,rate,ch,freq)
   assert sep>35,(fmt,rate,ch,sep)
   channels.append(dict(frequency_hz=freq,rms=rms,peak=peak,other_tone_rejection_db=sep))
  report.append(dict(format=['PCM8','PCM16','PCM24','PCM32','Float32'][fmt],rate=rate,channels=channels))
preview=array.array('h')
for fmt in range(5):
 case=fmt*3+2
 for mode in (0,1,2):
  count=12000
  for i in range(count):
   fade=min(1,i/480,(count-1-i)/480)
   for ch in (0,1):
    value=a[case*96000+2*(9600+i)+ch]/65536*fade if mode==2 or ch==mode else 0
    preview.append(round(value))
 for i in range(4800*2):preview.append(0)
if sys.byteorder!='little':preview.byteswap()
with wave.open(str(p/'qemu-uac1-preview.wav'),'wb') as w:
 w.setparams((2,2,48000,0,'NONE','not compressed'));w.writeframes(preview.tobytes())
(p/'analysis.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS 15 stereo cases: RMS, peak, 440/660 Hz pitch and >35 dB channel separation')
print('Preview: 4.25 seconds, five formats; each has left, right, then both, with faded edges.')

metadata = {
    'scope': 'Circle ARM software path; simulated USB packets; semihosted output',
    'head': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip(),
    'qemu': subprocess.check_output(['qemu-system-arm', '--version'], text=True).splitlines()[0],
    'compiler': subprocess.check_output(['arm-none-eabi-g++', '--version'], text=True).splitlines()[0],
    'image_sha256': hashlib.sha256((p/'kernel.img').read_bytes()).hexdigest(),
    'source_sha256': {str(s.relative_to(root)): hashlib.sha256(s.read_bytes()).hexdigest()
                     for s in (root/'lib/usb/gadget').glob('usbaudiogadget*.cpp')},
}
(p/'metadata.json').write_text(json.dumps(metadata, indent=2)+'\n')
if args.play:
    if sys.platform != 'darwin':
        raise SystemExit('--play requires macOS; WAV and validation are complete')
    player = p/'play-macos'
    subprocess.run(['clang', '-Wall', '-Wextra', '-Werror', str(here/'play-macos.c'),
                    '-framework', 'AudioToolbox', '-framework', 'CoreFoundation',
                    '-o', str(player)], check=True)
    subprocess.run([str(player), str(p/'qemu-uac1-preview.wav')], check=True)
