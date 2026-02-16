#
# Makefile
#

CIRCLEHOME = ../../../..

OBJS	= main.o kernel.o

LIBS	= $(CIRCLEHOME)/addon/gpio/libgpio.a \
	  $(CIRCLEHOME)/lib/libcircle.a

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
