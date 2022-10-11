#
# Makefile
#

CIRCLEHOME = ../../../..

OBJS	= main.o kernel.o

LIBS	= $(CIRCLEHOME)/addon/vc4/sound/libvchiqsound.a \
	  $(CIRCLEHOME)/addon/vc4/vchiq/libvchiq.a \
	  $(CIRCLEHOME)/addon/linux/liblinuxemu.a \
	  $(CIRCLEHOME)/lib/sched/libsched.a \
	  $(CIRCLEHOME)/lib/sound/libsound.a \
	  $(CIRCLEHOME)/lib/libcircle.a

EXTRACLEAN = sound.h

include $(CIRCLEHOME)/Rules.mk

kernel.o: sound.h

sound.h: sound.h.gz
	@echo "  UNZIP $@"
	@gunzip -c sound.h.gz > sound.h

-include $(DEPS)
