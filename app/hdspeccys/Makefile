#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= main.o \
          kernel.o \
          backgroundtask.o \
          tapetask.o \
          screenprocessortask.o          

LIBS	= $(CIRCLEHOME)/app/lib/zxutil/libzxutil.a \
          $(CIRCLEHOME)/app/lib/shell/libshell.a \
          $(CIRCLEHOME)/app/lib/zxtape/libzxtape.a \
          $(CIRCLEHOME)/app/lib/zxsmi/libzxsmi.a \
          $(CIRCLEHOME)/app/lib/zxscreen/libzxscreen.a \
          $(CIRCLEHOME)/lib/sched/libsched.a \
          $(CIRCLEHOME)/lib/libcircle.a

include ../Rules.mk

-include $(DEPS)
