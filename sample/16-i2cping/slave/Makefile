#
# Makefile
#

CIRCLEHOME = ../../..

OBJS	= main.o kernel.o i2cechoserver.o

LIBS	= $(CIRCLEHOME)/lib/libcircle.a

include ../../Rules.mk

ifeq ($(strip $(RASPPI)),4)
$(error RASPPI = 4 is not supported in i2cping/slave)
endif

-include $(DEPS)
