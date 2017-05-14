#
# Makefile
#
# Circle - A C++ bare metal environment for Raspberry Pi
# Copyright (C) 2015-2016  R. Stange <rsta2@o2online.de>
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

CIRCLEHOME = ../..

OBJS	= btsubsystem.o bttask.o \
	  btlogicallayer.o btinquiryresults.o \
	  bthcilayer.o btdevicemanager.o \
	  btqueue.o btuarttransport.o

libbluetooth.a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

EXTRACLEAN = BCM43430A1.h converttool

include $(CIRCLEHOME)/Rules.mk

%.h: %.hcd
	./converttool -b $< > $@

btdevicemanager.o: BCM43430A1.h

BCM43430A1.h: converttool

converttool: converttool.c
	gcc -o converttool converttool.c
