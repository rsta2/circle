#
# Makefile
#

CIRCLEHOME = ../../..

OBJS	= main.o kernel.o

LIBS	= $(CIRCLEHOME)/addon/qemu/libqemusupport.a \
	  $(CIRCLEHOME)/lib/libcircle.a

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
