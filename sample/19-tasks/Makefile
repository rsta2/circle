#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= main.o kernel.o screentask.o primetask.o ledtask.o

LIBS	= $(CIRCLEHOME)/lib/sched/libsched.a \
	  $(CIRCLEHOME)/lib/libcircle.a

include ../Rules.mk

-include $(DEPS)
