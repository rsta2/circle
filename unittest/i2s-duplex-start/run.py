#!/usr/bin/env python3
"""Exercise the BCM I2S startup path with simulated DMA allocation and registers."""
import os
from pathlib import Path
import subprocess
import tempfile

here = Path(__file__).resolve().parent
lib = here.parents[1] / "lib/sound"
dma = (lib / "dmasoundbuffers.cpp").read_text()
i2s = (lib / "i2ssoundbasedevice.cpp").read_text()
# Include Prepare when present so the same regression can run on the parent commit.
start = dma.find("boolean CDMASoundBuffers::Prepare (")
if start == -1:
    start = dma.index("boolean CDMASoundBuffers::Start (")
methods = dma[start:dma.index("void CDMASoundBuffers::Cancel (")]
methods += i2s[i2s.index("boolean CI2SSoundBaseDevice::Start ("):
               i2s.index("void CI2SSoundBaseDevice::Cancel (")]
with tempfile.TemporaryDirectory() as tmp:
    folder = Path(tmp)
    (folder / "startup.inc").write_text(methods)
    exe = folder / "test"
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-Wall", "-Wextra",
                    "-Werror", "-fsanitize=address,undefined", "-I" + tmp,
                    str(here / "test.cpp"), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
