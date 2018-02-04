#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= main.o kernel.o

LIBS	= $(CIRCLEHOME)/lib/libcircle$(RASPPI).a

EXTRACLEAN = sound.h

include ../Rules.mk

kernel.o: sound.h

sound.h: sound.h.gz
	gunzip -c sound.h.gz > sound.h
