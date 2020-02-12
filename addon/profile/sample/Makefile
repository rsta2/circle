#
# Makefile
#

CIRCLEHOME = ../../..

OBJS	= main.o kernel.o screentask.o primetask.o ledtask.o

LIBS	= $(CIRCLEHOME)/addon/profile/libprofile.a \
	  $(CIRCLEHOME)/addon/SDCard/libsdcard.a \
	  $(CIRCLEHOME)/addon/fatfs/libfatfs.a \
	  $(CIRCLEHOME)/lib/fs/fat/libfatfs.a \
	  $(CIRCLEHOME)/lib/fs/libfs.a \
	  $(CIRCLEHOME)/lib/sched/libsched.a \
	  $(CIRCLEHOME)/lib/libcircle.a

CFLAGS	+= -pg

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
