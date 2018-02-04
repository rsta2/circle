#
# Makefile
#

CIRCLEHOME = ../../../..

OBJS	= main.o kernel.o

LIBS	= $(CIRCLEHOME)/addon/vc4/sound/libvchiqsound$(RASPPI).a \
	  $(CIRCLEHOME)/addon/vc4/vchiq/libvchiq$(RASPPI).a \
	  $(CIRCLEHOME)/addon/linux/liblinuxemu$(RASPPI).a \
	  $(CIRCLEHOME)/lib/sched/libsched$(RASPPI).a \
	  $(CIRCLEHOME)/lib/libcircle$(RASPPI).a

EXTRACLEAN = sound.h

include $(CIRCLEHOME)/Rules.mk

kernel.o: sound.h

sound.h: sound.h.gz
	gunzip -c sound.h.gz > sound.h
