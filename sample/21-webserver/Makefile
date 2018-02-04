#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= main.o kernel.o webserver.o

LIBS	= $(CIRCLEHOME)/lib/usb/libusb$(RASPPI).a \
	  $(CIRCLEHOME)/lib/input/libinput$(RASPPI).a \
	  $(CIRCLEHOME)/lib/fs/libfs$(RASPPI).a \
	  $(CIRCLEHOME)/lib/net/libnet$(RASPPI).a \
	  $(CIRCLEHOME)/lib/sched/libsched$(RASPPI).a \
	  $(CIRCLEHOME)/lib/libcircle$(RASPPI).a

CONTENT	= index.h style.h ledoff.h ledon.h favicon.h

EXTRACLEAN = $(CONTENT) converttool

include ../Rules.mk

%.h: %.html
	./converttool $< > $@

%.h: %.css
	./converttool $< > $@

%.h: %.png
	./converttool -b $< > $@
	
%.h: %.ico
	./converttool -b $< > $@
	
webserver.o: $(CONTENT)

$(CONTENT): converttool

converttool: converttool.c
	gcc -o converttool converttool.c
