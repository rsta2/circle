#
# Makefile
#
# Circle - A C++ bare metal environment for Raspberry Pi
# Copyright (C) 2015-2019  R. Stange <rsta2@o2online.de>
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

OBJS	= netsubsystem.o nettask.o netsocket.o socket.o \
	  transportlayer.o networklayer.o linklayer.o netdevlayer.o phytask.o arphandler.o \
	  icmphandler.o routecache.o \
	  netconnection.o udpconnection.o \
	  tcpconnection.o retransmissionqueue.o retranstimeoutcalc.o tcprejector.o \
	  netconfig.o ipaddress.o netqueue.o checksumcalculator.o \
	  dnsclient.o ntpclient.o mqttclient.o mqttsendpacket.o mqttreceivepacket.o \
	  dhcpclient.o ntpdaemon.o httpdaemon.o httpclient.o tftpdaemon.o syslogdaemon.o

libnet.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
