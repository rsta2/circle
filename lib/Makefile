#
# Makefile
#
# Circle - A C++ bare metal environment for Raspberry Pi
# Copyright (C) 2014-2023  R. Stange <rsta2@o2online.de>
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

OBJS	= actled.o alloc.o assert.o bcmframebuffer.o bcmmailbox.o \
	  bcmpropertytags.o bcmwatchdog.o chargenerator.o classallocator.o \
	  cputhrottle.o debug.o delayloop.o device.o devicenameservice.o \
	  dmachannel.o gpioclock.o gpiomanager.o gpiopin.o gpiopinfiq.o \
	  i2cmaster.o i2cslave.o koptions.o \
	  logger.o machineinfo.o multicore.o nulldevice.o ptrarray.o ptrlist.o \
	  pwmoutput.o qemu.o screen.o serial.o \
	  spimaster.o spimasteraux.o spimasterdma.o spinlock.o \
	  string.o sysinit.o time.o timer.o tracer.o usertimer.o util.o \
	  util_fast.o virtualgpiopin.o chainboot.o macaddress.o netdevice.o \
	  new.o heapallocator.o pageallocator.o setjmp.o numberpool.o \
	  latencytester.o writebuffer.o 2dgraphics.o smimaster.o ptrlistfiq.o

OBJS32	= cache-v7.o exceptionhandler.o exceptionstub.o memory.o pagetable.o \
	  startup.o synchronize.o

OBJS64	= exceptionhandler64.o exceptionstub64.o memory64.o startup64.o \
	  synchronize64.o translationtable64.o

all: libcircle.a

include ../Rules.mk

ifeq ($(strip $(AARCH)),32)
OBJS	+= $(OBJS32)
ifeq ($(strip $(STDLIB_SUPPORT)),0)
OBJS	+= libhelper.o libstub.o
endif
else
OBJS	+= $(OBJS64)
endif

ifneq ($(strip $(RASPPI)),4)
OBJS	+= bcmrandom.o interrupt.o mphi.o
else
OBJS	+= bcm54213.o bcmpciehostbridge.o bcmrandom200.o interruptgic.o dma4channel.o \
	   devicetreeblob.o
endif

ifneq ($(strip $(STDLIB_SUPPORT)),3)
OBJS	+= purecall.o cxa_guard.o
endif

# using coherent memory region
ifeq ($(strip $(AARCH)),32)
CFLAGS += -mno-unaligned-access
else
CFLAGS += -mstrict-align
endif

libcircle.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

-include $(DEPS)
