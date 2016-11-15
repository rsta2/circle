#
# Makefile
#
# Circle - A C++ bare metal environment for Raspberry Pi
# Copyright (C) 2014-2016  R. Stange <rsta2@o2online.de>
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

OBJS	= actled.o alloc.o bcmmailbox.o bcmpropertytags.o gpiopin.o libhelper.o libstub.o \
	  memory.o new.o purecall.o synchronize.o sysinit.o timer.o util.o util_fast.o \
	  bcmframebuffer.o device.o koptions.o pagetable.o screen.o nulldevice.o \
	  assert.o chargenerator.o debug.o devicenameservice.o logger.o serial.o string.o \
	  delayloop.o exceptionhandler.o exceptionstub.o interrupt.o \
	  gpiomanager.o gpioclock.o dmachannel.o pwmsounddevice.o pwmoutput.o tracer.o \
	  i2cmaster.o i2cslave.o spimaster.o spimasterdma.o virtualgpiopin.o machineinfo.o \
	  multicore.o spinlock.o bcmrandom.o cache-v7.o cputhrottle.o \
	  ptrarray.o ptrlist.o time.o

all: startup.o libcircle.a

startup.o: startup.S

libcircle.a: $(OBJS)
	rm -f libcircle.a
	$(AR) cr libcircle.a $(OBJS)

include ../Rules.mk
