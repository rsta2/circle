#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= main.o kernel.o webserver.o

LIBS	= $(CIRCLEHOME)/lib/usb/libusb.a \
	  $(CIRCLEHOME)/lib/input/libinput.a \
	  $(CIRCLEHOME)/lib/fs/libfs.a \
	  $(CIRCLEHOME)/lib/net/libnet.a \
	  $(CIRCLEHOME)/lib/sched/libsched.a \
	  $(CIRCLEHOME)/lib/libcircle.a

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
