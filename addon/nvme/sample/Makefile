#
# Makefile
#

CIRCLEHOME = ../../..

OBJS	= main.o kernel.o ledtask.o

LIBS	= $(CIRCLEHOME)/addon/nvme/libnvme.a \
	  $(CIRCLEHOME)/lib/fs/libfs.a \
	  $(CIRCLEHOME)/lib/fs/fat/libfatfs.a \
	  $(CIRCLEHOME)/addon/fatfs/libfatfs.a \
	  $(CIRCLEHOME)/lib/sched/libsched.a \
	  $(CIRCLEHOME)/lib/libcircle.a

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
