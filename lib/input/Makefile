#
# Makefile
#
# Circle - A C++ bare metal environment for Raspberry Pi
# Copyright (C) 2014-2024  R. Stange <rsta2@o2online.de>
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

OBJS	= keyboardbehaviour.o keymap.o mousebehaviour.o mouse.o \
	  touchscreen.o rpitouchscreen.o xpt2046touchscreen.o \
	  console.o keyboardbuffer.o linediscipline.o

libinput.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
