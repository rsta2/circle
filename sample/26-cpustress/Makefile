#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= main.o kernel.o mandelbrot.o temptask.o

LIBS	= $(CIRCLEHOME)/lib/sched/libsched.a \
	  $(CIRCLEHOME)/lib/libcircle.a

include ../Rules.mk

-include $(DEPS)
