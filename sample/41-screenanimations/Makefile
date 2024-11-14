#
# Makefile
#

#USE_ST7789 = 1

CIRCLEHOME = ../..

OBJS	= main.o kernel.o graphicshape.o

ifeq ($(strip $(USE_ST7789)),1)
LIBS	+= $(CIRCLEHOME)/addon/display/libdisplay.a

CFLAGS	+= -DUSE_ST7789=$(USE_ST7789)
endif

LIBS	+= $(CIRCLEHOME)/lib/libcircle.a

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
