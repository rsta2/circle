This sample showcases every GDB File I/O functions and how to print Circle logs
into the GDB console.

To compile it, you will need to download Alpha for the Raspberry Pi
(https://github.com/farjump/raspberry-pi) to use and link the File I/O library,
and install arm-none-eabi-gdb.

Use the following `Config.mk` file to compile Circle:
```
ALPHA := <path to alpha for rpi>
CFLAGS += -I$(ALPHA)/sdk/libalpha/include -I$(CIRCLEHOME) -DUSE_ALPHA_STUB_AT=0x7F00000
LIBS += $(CIRCLEHOME)/addon/alpha/libaddonalpha.a $(ALPHA)/sdk/libfileio.a
OPTIMIZE := -O0 -g3 -ggdb
RASPPI := <your rpi model number>
```

To run it, adapt `run.gdb` and use GDB to run it:
```
$ arm-none-eabi-gdb -ex 'source -v run.gdb' kernel<version>.elf
```