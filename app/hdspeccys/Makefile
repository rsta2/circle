#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= main.o kernel.o

LIBS	= $(CIRCLEHOME)/app/lib/zxutil/libzxutil.a \
          $(CIRCLEHOME)/app/lib/zxdma/libzxdma.a \
          $(CIRCLEHOME)/app/lib/zxsmimaster/libzxsmimaster.a \
          $(CIRCLEHOME)/app/lib/shell/libshell.a \
          $(CIRCLEHOME)/app/lib/zxsmi/libzxsmi.a \
          $(CIRCLEHOME)/app/lib/zxscreen/libzxscreen.a \
          $(CIRCLEHOME)/lib/libcircle.a

include ../Rules.mk

-include $(DEPS)
