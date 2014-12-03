#
# Rules.mk
#
# Circle - A C++ bare metal environment for Raspberry Pi
# Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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

PREFIX	?= arm-rpi-linux-gnueabi-

CC	= $(PREFIX)gcc
CPP	= $(PREFIX)g++
AS	= $(CC)
LD	= $(PREFIX)ld
AR	= $(PREFIX)ar

AFLAGS	+= -I $(CIRCLEHOME)/include
CFLAGS	+= -march=armv6 -mtune=arm1176jzf-s -Wall -Wno-psabi -fsigned-char -fno-builtin -nostdinc -nostdlib \
	   -undef -D__circle__ -I $(CIRCLEHOME)/include -I $(CIRCLEHOME)/addon -O #-DNDEBUG
CPPFLAGS+= $(CFLAGS) -fno-exceptions -fno-rtti -std=c++0x

%.o: %.S
	$(AS) $(AFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CPP) $(CPPFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.a *.elf *.lst *.img *.cir *.map *~ $(EXTRACLEAN)
