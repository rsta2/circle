#
# Rules.mk
#
# Circle - A C++ bare metal environment for Raspberry Pi
# Copyright (C) 2014-2019  R. Stange <rsta2@o2online.de>
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

CIRCLEHOME ?= ..

-include $(CIRCLEHOME)/Config.mk
-include $(CIRCLEHOME)/Config2.mk	# is not overwritten by "configure"

AARCH	 ?= 32
RASPPI	 ?= 1
PREFIX	 ?= arm-eabi-
PREFIX64 ?= aarch64-elf-

# see: doc/stdlib-support.txt
STDLIB_SUPPORT ?= 1

# set this to "softfp" if you want to link specific libraries
FLOAT_ABI ?= hard

CC	= $(PREFIX)gcc
CPP	= $(PREFIX)g++
AS	= $(CC)
LD	= $(PREFIX)ld
AR	= $(PREFIX)ar

ifeq ($(strip $(AARCH)),32)
ifeq ($(strip $(RASPPI)),1)
ARCH	?= -DAARCH=32 -march=armv6k -mtune=arm1176jzf-s -marm -mfpu=vfp -mfloat-abi=$(FLOAT_ABI)
TARGET	?= kernel
else ifeq ($(strip $(RASPPI)),2)
ARCH	?= -DAARCH=32 -march=armv7-a -marm -mfpu=neon-vfpv4 -mfloat-abi=$(FLOAT_ABI)
TARGET	?= kernel7
else ifeq ($(strip $(RASPPI)),3)
ARCH	?= -DAARCH=32 -march=armv8-a -mtune=cortex-a53 -marm -mfpu=neon-fp-armv8 -mfloat-abi=$(FLOAT_ABI)
TARGET	?= kernel8-32
else ifeq ($(strip $(RASPPI)),4)
ARCH	?= -DAARCH=32 -march=armv8-a -mtune=cortex-a72 -marm -mfpu=neon-fp-armv8 -mfloat-abi=$(FLOAT_ABI)
TARGET	?= kernel7l
else
$(error RASPPI must be set to 1, 2, 3 or 4)
endif
LOADADDR = 0x8000
else ifeq ($(strip $(AARCH)),64)
ifeq ($(strip $(RASPPI)),3)
ARCH	?= -DAARCH=64 -march=armv8-a -mtune=cortex-a53 -mlittle-endian -mcmodel=small
TARGET	?= kernel8
else ifeq ($(strip $(RASPPI)),4)
ARCH	?= -DAARCH=64 -march=armv8-a -mtune=cortex-a72 -mlittle-endian -mcmodel=small
TARGET	?= kernel8-rpi4
else
$(error RASPPI must be set to 3 or 4)
endif
PREFIX	= $(PREFIX64)
LOADADDR = 0x80000
else
$(error AARCH must be set to 32 or 64)
endif

ifneq ($(strip $(STDLIB_SUPPORT)),0)
MAKE_VERSION_MAJOR := $(firstword $(subst ., ,$(MAKE_VERSION)))
ifneq ($(filter 0 1 2 3,$(MAKE_VERSION_MAJOR)),)
$(error STDLIB_SUPPORT > 0 requires GNU make 4.0 or newer)
endif
endif

ifeq ($(strip $(STDLIB_SUPPORT)),3)
LIBSTDCPP != $(CPP) $(ARCH) -print-file-name=libstdc++.a
EXTRALIBS += $(LIBSTDCPP)
LIBGCC_EH != $(CPP) $(ARCH) -print-file-name=libgcc_eh.a
ifneq ($(strip $(LIBGCC_EH)),libgcc_eh.a)
EXTRALIBS += $(LIBGCC_EH)
endif
ifeq ($(strip $(AARCH)),64)
CRTBEGIN != $(CPP) $(ARCH) -print-file-name=crtbegin.o
CRTEND   != $(CPP) $(ARCH) -print-file-name=crtend.o
endif
else
CPPFLAGS  += -fno-exceptions -fno-rtti -nostdinc++
endif

ifeq ($(strip $(STDLIB_SUPPORT)),0)
CFLAGS	  += -nostdinc
else
LIBGCC	  != $(CPP) $(ARCH) -print-file-name=libgcc.a
EXTRALIBS += $(LIBGCC)
endif

ifeq ($(strip $(STDLIB_SUPPORT)),1)
LIBM	  != $(CPP) $(ARCH) -print-file-name=libm.a
ifneq ($(strip $(LIBM)),libm.a)
EXTRALIBS += $(LIBM)
endif
endif

OPTIMIZE ?= -O2

INCLUDE	+= -I $(CIRCLEHOME)/include -I $(CIRCLEHOME)/addon -I $(CIRCLEHOME)/app/lib \
	   -I $(CIRCLEHOME)/addon/vc4 -I $(CIRCLEHOME)/addon/vc4/interface/khronos/include
DEFINE	+= -D__circle__ -DRASPPI=$(RASPPI) -DSTDLIB_SUPPORT=$(STDLIB_SUPPORT) \
	   -D__VCCOREVER__=0x04000000 -U__unix__ -U__linux__ #-DNDEBUG

AFLAGS	+= $(ARCH) $(DEFINE) $(INCLUDE) $(OPTIMIZE)
CFLAGS	+= $(ARCH) -Wall -fsigned-char -ffreestanding $(DEFINE) $(INCLUDE) $(OPTIMIZE) -g
CPPFLAGS+= $(CFLAGS) -std=c++14

%.o: %.S
	@echo "  AS    $@"
	@$(AS) $(AFLAGS) -c -o $@ $<

%.o: %.c
	@echo "  CC    $@"
	@$(CC) $(CFLAGS) -std=gnu99 -c -o $@ $<

%.o: %.cpp
	@echo "  CPP   $@"
	@$(CPP) $(CPPFLAGS) -c -o $@ $<

$(TARGET).img: $(OBJS) $(LIBS) $(CIRCLEHOME)/circle.ld
	@echo "  LD    $(TARGET).elf"
	@$(LD) -o $(TARGET).elf -Map $(TARGET).map --section-start=.init=$(LOADADDR) \
		-T $(CIRCLEHOME)/circle.ld $(CRTBEGIN) $(OBJS) \
		--start-group $(LIBS) $(EXTRALIBS) --end-group $(CRTEND)
	@echo "  DUMP  $(TARGET).lst"
	@$(PREFIX)objdump -d $(TARGET).elf | $(PREFIX)c++filt > $(TARGET).lst
	@echo "  COPY  $(TARGET).img"
	@$(PREFIX)objcopy $(TARGET).elf -O binary $(TARGET).img
	@echo -n "  WC    $(TARGET).img => "
	@wc -c < $(TARGET).img

clean:
	rm -f *.o *.a *.elf *.lst *.img *.hex *.cir *.map *~ $(EXTRACLEAN)

ifneq ($(strip $(SDCARD)),)
install: $(TARGET).img
	cp $(TARGET).img $(SDCARD)
	sync
endif

#
# Eclipse support
#

SERIALPORT  ?= /dev/ttyUSB0
USERBAUD ?= 115200
FLASHBAUD ?= 115200
REBOOTMAGIC ?=

$(TARGET).hex: $(TARGET).img
	@echo "  COPY  $(TARGET).hex"
	@$(PREFIX)objcopy $(TARGET).elf -O ihex $(TARGET).hex

flash: $(TARGET).hex
ifneq ($(strip $(REBOOTMAGIC)),)
	python $(CIRCLEHOME)/tools/reboottool.py $(REBOOTMAGIC) $(SERIALPORT) $(USERBAUD)
endif
	python $(CIRCLEHOME)/tools/flasher.py $(TARGET).hex $(SERIALPORT) $(FLASHBAUD)

monitor:
	putty -serial $(SERIALPORT) -sercfg $(USERBAUD)
