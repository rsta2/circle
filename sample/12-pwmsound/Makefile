#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= main.o kernel.o

LIBS	= $(CIRCLEHOME)/lib/libcircle.a

EXTRACLEAN = sound.h

include ../Rules.mk

kernel.o: sound.h

sound.h: sound.h.gz
	gunzip -k sound.h.gz
