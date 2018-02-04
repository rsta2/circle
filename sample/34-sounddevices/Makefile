#
# Makefile
#

#USE_VCHIQ_SOUND = 1

CIRCLEHOME = ../..

OBJS	= main.o kernel.o oscillator.o

ifeq ($(strip $(USE_VCHIQ_SOUND)),1)
LIBS	= $(CIRCLEHOME)/addon/vc4/sound/libvchiqsound$(RASPPI).a \
	  $(CIRCLEHOME)/addon/vc4/vchiq/libvchiq$(RASPPI).a \
	  $(CIRCLEHOME)/addon/linux/liblinuxemu$(RASPPI).a

CFLAGS += -DUSE_VCHIQ_SOUND=$(USE_VCHIQ_SOUND)
endif

LIBS	+= $(CIRCLEHOME)/lib/sched/libsched$(RASPPI).a \
	   $(CIRCLEHOME)/lib/libcircle$(RASPPI).a

include ../Rules.mk
