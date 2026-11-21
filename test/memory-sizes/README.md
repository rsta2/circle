# Memory sizes taken from the RAM size

Run from the Circle root with Python 3 and a C++17 compiler that supports AddressSanitizer and UndefinedBehaviorSanitizer:

```sh
python3 test/memory-sizes/run.py
```

`CXX` selects the compiler. `--source /path/to/circle` tests another checkout with this same harness. No Raspberry Pi is needed.

The test compiles the actual `lib/memory64.cpp` with the actual headers of `include/`. `run.py` takes out `CMemorySystem::Destructor ()` and `EnableMMU ()`, which read and write Arm system registers, and turns the remaining `asm` statements into no-ops, since they run only with the MMU enabled. Stand-ins replace the heap and page allocators, the translation table, the property tags, the machine info, the barriers and the spin lock. The heap allocator stand-in notes the start and the size each heap is set up with.

For 1, 2, 4, 8 and 16 GB of RAM, the test compares the sizes with a table of expected values in `test.cpp`. The high memory and the high heap are the RAM above 1 GB, limited to the high memory region. With `KASAN_SUPPORTED`, the RAM is what `GetRAMSizeEarly ()` reports, at most 4 GB, for the high memory as well. The shadow memory is an eighth of it, and the low heap starts behind it. The stand-in returns 2048 from `GetRAMSizeEarly ()` on `RASPPI=5`, as `lib/machineinfo.cpp` does, so `aarch64-rpi5-kasan` checks only the shadow memory and the low heap.

It runs three builds:

| Build | Defines |
| --- | --- |
| `aarch64-rpi5` | `AARCH=64 RASPPI=5` |
| `aarch64-rpi4-kasan` | `AARCH=64 RASPPI=4 KASAN_SUPPORTED` |
| `aarch64-rpi5-kasan` | `AARCH=64 RASPPI=5 KASAN_SUPPORTED` |

`lib/memory.cpp` (AArch32) is not part of the host test. Its `types.h` defines `size_t` and `uintptr` with 32 bits, which a 64-bit host compiler does not accept. Its shadow memory size is the same expression as in `lib/memory64.cpp` and gets the same fix. AArch32 builds go up to `RASPPI=4`, whose high memory region is 2 GB, and even the wrapped high memory size of 3 GB is above that, so the change to the high memory size there gives the same result as before.

## Results

Tested on macOS (arm64) with Apple Clang and ASan/UBSan against `develop` 16f4df33:

| Source | Passed | Failed |
| --- | ---: | ---: |
| Unmodified develop | 30 | 10 |
| With this fix | 40 | 0 |

Unmodified, `aarch64-rpi5` reports 3072 MB of high memory for 8 and 16 GB instead of 7168 MB, and `aarch64-rpi4-kasan` reports no shadow memory for 4 GB and more, with the low heap starting at 9 MB instead of 521 MB. `aarch64-rpi5-kasan` passes both ways. The harness also passes with `CXX=g++-16`.

`lib/memory.cpp` with the fix also builds with `arm-none-eabi-gcc` 16.2.0 for `AARCH=32` and `RASPPI=2`, `3` and `4`, and for `RASPPI=4` with `KASAN_ENABLED=1`.
