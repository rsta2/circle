#
# Makefile
#

#USE_FATFS = 1

CIRCLEHOME = ../../..

ifeq ($(strip $(USE_FATFS)),1)
CFLAGS += -DUSE_FATFS
endif

OBJS	= main.o kernel.o

ifeq ($(strip $(USE_FATFS)),1)
LIBS	= ../libproperties.a \
	  $(CIRCLEHOME)/addon/SDCard/libsdcard.a \
	  $(CIRCLEHOME)/addon/fatfs/libfatfs.a \
	  $(CIRCLEHOME)/lib/fs/libfs.a \
	  $(CIRCLEHOME)/lib/libcircle.a
else
LIBS	= ../libproperties.a \
	  $(CIRCLEHOME)/addon/SDCard/libsdcard.a \
	  $(CIRCLEHOME)/lib/fs/fat/libfatfs.a \
	  $(CIRCLEHOME)/lib/fs/libfs.a \
	  $(CIRCLEHOME)/lib/libcircle.a
endif

include $(CIRCLEHOME)/sample/Rules.mk

-include $(DEPS)
