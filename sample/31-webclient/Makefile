#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= main.o kernel.o htmlscanner.o

LIBS	= $(CIRCLEHOME)/lib/usb/libusb$(RASPPI).a \
	  $(CIRCLEHOME)/lib/input/libinput$(RASPPI).a \
	  $(CIRCLEHOME)/lib/fs/libfs$(RASPPI).a \
	  $(CIRCLEHOME)/lib/net/libnet$(RASPPI).a \
	  $(CIRCLEHOME)/lib/sched/libsched$(RASPPI).a \
	  $(CIRCLEHOME)/lib/libcircle$(RASPPI).a

include ../Rules.mk
