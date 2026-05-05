#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= main.o kernel.o readertask.o writertask.o

LIBS	= $(CIRCLEHOME)/lib/sched/libsched.a \
	  $(CIRCLEHOME)/lib/libcircle.a

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
