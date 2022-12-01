#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= main.o kernel.o

LIBS	= $(CIRCLEHOME)/lib/sound/libsound.a \
	  $(CIRCLEHOME)/lib/libcircle.a

EXTRACLEAN = sound.h

include ../Rules.mk

kernel.o: sound.h

sound.h: sound.h.gz
	@echo "  UNZIP $@"
	@gunzip -c sound.h.gz > sound.h

-include $(DEPS)
