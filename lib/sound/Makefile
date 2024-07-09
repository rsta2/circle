#
# Makefile
#
# Circle - A C++ bare metal environment for Raspberry Pi
# Copyright (C) 2022-2024  R. Stange <rsta2@o2online.de>
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

all: libsound.a

include $(CIRCLEHOME)/Rules.mk

OBJS	= soundbasedevice.o pcm512xsoundcontroller.o wm8960soundcontroller.o pwmsounddevice.o

ifneq ($(strip $(RASPPI)),5)
OBJS	+= dmasoundbuffers.o hdmisoundbasedevice.o i2ssoundbasedevice.o \
	   pwmsoundbasedevice.o
else
OBJS	+= i2ssoundbasedevice-rp1.o pwmsoundbasedevice-rp1.o
endif

ifneq ($(filter 4 5,$(RASPPI)),)
OBJS	+= usbsoundbasedevice.o usbsoundcontroller.o
endif

libsound.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

-include $(DEPS)
