#
# Rules.mk
#
# Circle - A C++ bare metal environment for Raspberry Pi
# Copyright (C) 2014-2017  R. Stange <rsta2@o2online.de>
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

ifeq ($(strip $(CIRCLEHOME)),)
CIRCLEHOME = ..
endif

-include $(CIRCLEHOME)/Config.mk

RASPPI	?= 1
PREFIX	?= arm-none-eabi-

CC	= $(PREFIX)gcc
CPP	= $(PREFIX)g++
AS	= $(CC)
LD	= $(PREFIX)ld
AR	= $(PREFIX)ar

ifeq ($(strip $(RASPPI)),1)
ARCH	?= -march=armv6k -mtune=arm1176jzf-s -mfloat-abi=hard
TARGET	?= kernel
else ifeq ($(strip $(RASPPI)),2)
ARCH	?= -march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard
TARGET	?= kernel7
else
ARCH	?= -march=armv8-a -mtune=cortex-a53 -mfpu=neon-fp-armv8 -mfloat-abi=hard
TARGET	?= kernel8-32
endif

LIBGCC	  != $(CPP) -mfloat-abi=hard -print-file-name=libgcc.a
LIBGCC_EH != $(CPP) -mfloat-abi=hard -print-file-name=libgcc_eh.a
LIBSTDCPP != $(CPP) -mfloat-abi=hard -print-file-name=libstdc++.a

ifeq ($(strip $(STDLIB_SUPPORT)),1)
AFLAGS	+= -DSTDLIB_SUPPORT
CFLAGS	+= -DSTDLIB_SUPPORT
EXTRALIBS += $(LIBSTDCPP)
ifneq ($(strip $(LIBGCC_EH)),libgcc_eh.a)
EXTRALIBS += $(LIBGCC_EH)
endif
else
CPPFLAGS+= -fno-exceptions -fno-rtti
endif

OPTIMIZE ?= -O2

INCLUDE	+= -I $(CIRCLEHOME)/include -I $(CIRCLEHOME)/addon -I $(CIRCLEHOME)/app/lib
EXTRALIBS += $(LIBGCC)

AFLAGS	+= $(ARCH) -DRASPPI=$(RASPPI) $(INCLUDE)
CFLAGS	+= $(ARCH) -Wall -fsigned-char -ffreestanding \
	   -D__circle__ -DRASPPI=$(RASPPI) $(INCLUDE) $(OPTIMIZE) -g #-DNDEBUG
CPPFLAGS+= $(CFLAGS) -std=c++14

%.o: %.S
	$(AS) $(AFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CPP) $(CPPFLAGS) -c -o $@ $<

$(TARGET).img: $(OBJS) $(LIBS) $(CIRCLEHOME)/lib/startup.o $(CIRCLEHOME)/circle.ld
	$(LD) -o $(TARGET).elf -Map $(TARGET).map -T $(CIRCLEHOME)/circle.ld $(CIRCLEHOME)/lib/startup.o $(OBJS) $(EXTRALIBS) $(LIBS) $(EXTRALIBS)
	$(PREFIX)objdump -d $(TARGET).elf | $(PREFIX)c++filt > $(TARGET).lst
	$(PREFIX)objcopy $(TARGET).elf -O binary $(TARGET).img
	wc -c $(TARGET).img

clean:
	rm -f *.o *.a *.elf *.lst *.img *.cir *.map *~ $(EXTRACLEAN)
