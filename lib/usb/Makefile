#
# Makefile
#
# Circle - A C++ bare metal environment for Raspberry Pi
# Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
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

all: libusb.a

include $(CIRCLEHOME)/Rules.mk

OBJS	= lan7800.o smsc951x.o usbbluetooth.o usbcdcethernet.o \
	  usbconfigparser.o usbdevice.o usbdevicefactory.o usbendpoint.o usbfunction.o \
	  usbgamepad.o usbgamepadps3.o usbgamepadps4.o usbgamepadstandard.o usbgamepadswitchpro.o \
	  usbgamepadxbox360.o usbgamepadxboxone.o usbhiddevice.o usbhostcontroller.o \
	  usbkeyboard.o usbmassdevice.o usbmidi.o usbmouse.o usbprinter.o usbrequest.o \
	  usbstandardhub.o usbstring.o usbserial.o usbserialch341.o usbserialcp2102.o \
	  usbserialpl2303.o usbserialft231x.o usbserialcdc.o

ifneq ($(strip $(RASPPI)),4)
OBJS	+= dwhcidevice.o dwhciframeschednper.o dwhciframeschednsplit.o dwhciframeschedper.o \
	   dwhciregister.o dwhcirootport.o dwhcixactqueue.o dwhcixferstagedata.o
else
OBJS	+= xhcicommandmanager.o xhcidevice.o xhciendpoint.o xhcieventmanager.o xhcimmiospace.o \
	   xhciring.o xhciroothub.o xhcirootport.o xhcisharedmemallocator.o xhcislotmanager.o \
	   xhciusbdevice.o
endif

libusb.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

-include $(DEPS)
