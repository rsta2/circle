#
# Makefile
#

CIRCLEHOME = ../../..

OBJS	= main.o kernel.o

LIBS	= ../libproperties$(RASPPI).a \
	  $(CIRCLEHOME)/addon/SDCard/libsdcard$(RASPPI).a \
	  $(CIRCLEHOME)/lib/fs/fat/libfatfs$(RASPPI).a \
	  $(CIRCLEHOME)/lib/fs/libfs$(RASPPI).a \
	  $(CIRCLEHOME)/lib/libcircle$(RASPPI).a

include $(CIRCLEHOME)/sample/Rules.mk
