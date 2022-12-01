#
# Makefile
#

#USE_VCHIQ_SOUND = 1

CIRCLEHOME = ../..

OBJS	= main.o kernel.o oscillator.o

ifeq ($(strip $(USE_VCHIQ_SOUND)),1)
LIBS	= $(CIRCLEHOME)/addon/vc4/sound/libvchiqsound.a \
	  $(CIRCLEHOME)/addon/vc4/vchiq/libvchiq.a \
	  $(CIRCLEHOME)/addon/linux/liblinuxemu.a

CFLAGS += -DUSE_VCHIQ_SOUND=$(USE_VCHIQ_SOUND)
endif

LIBS	+= $(CIRCLEHOME)/lib/sched/libsched.a \
	   $(CIRCLEHOME)/lib/sound/libsound.a \
	   $(CIRCLEHOME)/lib/usb/libusb.a \
	   $(CIRCLEHOME)/lib/input/libinput.a \
	   $(CIRCLEHOME)/lib/fs/libfs.a \
	   $(CIRCLEHOME)/lib/libcircle.a

include ../Rules.mk

-include $(DEPS)
