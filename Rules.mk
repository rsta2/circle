#
# Rules.mk
#
# Circle - A C++ bare metal environment for Raspberry Pi
# Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
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
PREFIX	 ?= arm-none-eabi-
PREFIX64 ?= aarch64-none-elf-

# see: doc/stdlib-support.txt
STDLIB_SUPPORT ?= 1

# set this to 0 to globally disable dependency checking
CHECK_DEPS ?= 1

# set this to "softfp" if you want to link specific libraries
FLOAT_ABI ?= hard

# set this to 1 to enable garbage collection on sections, may cause side effects
GC_SECTIONS ?= 0

CC	= $(PREFIX)gcc
CPP	= $(PREFIX)g++
AS	= $(CC)
LD	= $(PREFIX)ld
AR	= $(PREFIX)ar

ifeq ($(strip $(AARCH)),32)
ifeq ($(strip $(RASPPI)),1)
ARCH	?= -DAARCH=32 -mcpu=arm1176jzf-s -marm -mfpu=vfp -mfloat-abi=$(FLOAT_ABI)
TARGET	?= kernel
else ifeq ($(strip $(RASPPI)),2)
ARCH	?= -DAARCH=32 -mcpu=cortex-a7 -marm -mfpu=neon-vfpv4 -mfloat-abi=$(FLOAT_ABI)
TARGET	?= kernel7
else ifeq ($(strip $(RASPPI)),3)
ARCH	?= -DAARCH=32 -mcpu=cortex-a53 -marm -mfpu=neon-fp-armv8 -mfloat-abi=$(FLOAT_ABI)
TARGET	?= kernel8-32
else ifeq ($(strip $(RASPPI)),4)
ARCH	?= -DAARCH=32 -mcpu=cortex-a72 -marm -mfpu=neon-fp-armv8 -mfloat-abi=$(FLOAT_ABI)
TARGET	?= kernel7l
else
$(error RASPPI must be set to 1, 2, 3 or 4)
endif
LOADADDR = 0x8000
else ifeq ($(strip $(AARCH)),64)
ifeq ($(strip $(RASPPI)),3)
ARCH	?= -DAARCH=64 -mcpu=cortex-a53 -mlittle-endian -mcmodel=small
TARGET	?= kernel8
else ifeq ($(strip $(RASPPI)),4)
ARCH	?= -DAARCH=64 -mcpu=cortex-a72 -mlittle-endian -mcmodel=small
TARGET	?= kernel8-rpi4
else
$(error RASPPI must be set to 3 or 4)
endif
PREFIX	= $(PREFIX64)
LOADADDR = 0x80000
else
$(error AARCH must be set to 32 or 64)
endif

ifeq ($(strip $(STDLIB_SUPPORT)),3)
LIBSTDCPP = "$(shell $(CPP) $(ARCH) -print-file-name=libstdc++.a)"
EXTRALIBS += $(LIBSTDCPP)
LIBGCC_EH = "$(shell $(CPP) $(ARCH) -print-file-name=libgcc_eh.a)"
ifneq ($(strip $(LIBGCC_EH)),"libgcc_eh.a")
EXTRALIBS += $(LIBGCC_EH)
endif
ifeq ($(strip $(AARCH)),64)
CRTBEGIN = "$(shell $(CPP) $(ARCH) -print-file-name=crtbegin.o)"
CRTEND   = "$(shell $(CPP) $(ARCH) -print-file-name=crtend.o)"
endif
else
CPPFLAGS  += -fno-exceptions -fno-rtti -nostdinc++
endif

ifeq ($(strip $(STDLIB_SUPPORT)),0)
CFLAGS	  += -nostdinc
else
LIBGCC	  = "$(shell $(CPP) $(ARCH) -print-file-name=libgcc.a)"
EXTRALIBS += $(LIBGCC)
endif

ifeq ($(strip $(STDLIB_SUPPORT)),1)
LIBM	  = "$(shell $(CPP) $(ARCH) -print-file-name=libm.a)"
ifneq ($(strip $(LIBM)),"libm.a")
EXTRALIBS += $(LIBM)
endif
endif

ifeq ($(strip $(GC_SECTIONS)),1)
CFLAGS	+= -ffunction-sections -fdata-sections
LDFLAGS	+= --gc-sections
endif

OPTIMIZE ?= -O2
STANDARD ?= -std=c++14 -Wno-aligned-new

INCLUDE	+= -I $(CIRCLEHOME)/include -I $(CIRCLEHOME)/addon -I $(CIRCLEHOME)/app/lib \
	   -I $(CIRCLEHOME)/addon/vc4 -I $(CIRCLEHOME)/addon/vc4/interface/khronos/include
DEFINE	+= -D__circle__ -DRASPPI=$(RASPPI) -DSTDLIB_SUPPORT=$(STDLIB_SUPPORT) \
	   -D__VCCOREVER__=0x04000000 -U__unix__ -U__linux__ #-DNDEBUG

AFLAGS	+= $(ARCH) $(DEFINE) $(INCLUDE) $(OPTIMIZE)
CFLAGS	+= $(ARCH) -Wall -fsigned-char -ffreestanding $(DEFINE) $(INCLUDE) $(OPTIMIZE) -g
CPPFLAGS+= $(CFLAGS) $(STANDARD)
LDFLAGS	+= --section-start=.init=$(LOADADDR)

ifeq ($(strip $(CHECK_DEPS)),1)
DEPS	= $(OBJS:.o=.d)
endif

%.o: %.S
	@echo "  AS    $@"
	@$(AS) $(AFLAGS) -c -o $@ $<

%.o: %.c
	@echo "  CC    $@"
	@$(CC) $(CFLAGS) -std=gnu99 -c -o $@ $<

%.o: %.cpp
	@echo "  CPP   $@"
	@$(CPP) $(CPPFLAGS) -c -o $@ $<

%.d: %.S
	@$(AS) $(AFLAGS) -M -MG -MT $*.o -MT $@ -MF $@ $<

%.d: %.c
	@$(CC) $(CFLAGS) -M -MG -MT $*.o -MT $@ -MF $@ $<

%.d: %.cpp
	@$(CPP) $(CPPFLAGS) -M -MG -MT $*.o -MT $@ -MF $@ $<

$(TARGET).img: $(OBJS) $(LIBS) $(CIRCLEHOME)/circle.ld
	@echo "  LD    $(TARGET).elf"
	@$(LD) -o $(TARGET).elf -Map $(TARGET).map $(LDFLAGS) \
		-T $(CIRCLEHOME)/circle.ld $(CRTBEGIN) $(OBJS) \
		--start-group $(LIBS) $(EXTRALIBS) --end-group $(CRTEND)
	@echo "  DUMP  $(TARGET).lst"
	@$(PREFIX)objdump -d $(TARGET).elf | $(PREFIX)c++filt > $(TARGET).lst
	@echo "  COPY  $(TARGET).img"
	@$(PREFIX)objcopy $(TARGET).elf -O binary $(TARGET).img
	@echo -n "  WC    $(TARGET).img => "
	@wc -c < $(TARGET).img

clean:
	@echo "  CLEAN " `pwd`
	@rm -f *.d *.o *.a *.elf *.lst *.img *.hex *.cir *.map *~ $(EXTRACLEAN)

ifneq ($(strip $(SDCARD)),)
install: $(TARGET).img
	cp $(TARGET).img $(SDCARD)
	sync
endif

ifneq ($(strip $(TFTPHOST)),)
tftpboot: $(TARGET).img
	tftp -m binary $(TFTPHOST) -c put $(TARGET).img
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

# Command line to run node and python.  
# Including the '.exe' forces WSL to run the Windows host version
# of these commands.  If putty and node are available on the windows 
# machine we can get around WSL's lack of serial port support
ifeq ($(strip $(WSL_DISTRO_NAME)),)
NODE=node 
PUTTY=putty
PUTTYSERIALPORT=$(SERIALPORT)
else
NODE=node.exe
PUTTY=putty.exe
PUTTYSERIALPORT=$(subst /dev/ttyS,COM,$(SERIALPORT))		# Remap to windows name
endif

ifeq ($(strip $(USEFLASHY)),)

# Flash with python
flash: $(TARGET).hex
ifneq ($(strip $(REBOOTMAGIC)),)
	python3 $(CIRCLEHOME)/tools/reboottool.py $(REBOOTMAGIC) $(SERIALPORT) $(USERBAUD)
endif
	python3 $(CIRCLEHOME)/tools/flasher.py $(TARGET).hex $(SERIALPORT) $(FLASHBAUD)

else

# Flash with flashy
flash: $(TARGET).hex
	$(NODE) $(CIRCLEHOME)/tools/flashy/flashy.js \
		$(SERIALPORT) \
		--flashBaud:$(FLASHBAUD) \
		--userBaud:$(USERBAUD) \
		--reboot:$(REBOOTMAGIC) \
		$(FLASHYFLAGS) \
		$(TARGET).hex

endif

# Monitor in putty
monitor:
	$(PUTTY) -serial $(PUTTYSERIALPORT) -sercfg $(USERBAUD)

# Monitor in terminal (Linux only)
cat:
	stty -F $(SERIALPORT) $(USERBAUD) cs8 -cstopb -parenb -icrnl
	cat $(SERIALPORT)
