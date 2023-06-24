#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= main.o \
          kernel.o \
          backgroundtask.o \
          tapetask.o \
          screenprocessortask.o          

LIBS	= $(CIRCLEHOME)/app/lib/zxutil/libzxutil.a \
          $(CIRCLEHOME)/app/lib/shell/libshell.a \
          $(CIRCLEHOME)/app/lib/zxweb/libzxweb.a \
          $(CIRCLEHOME)/app/lib/zxtape/libzxtape.a \
          $(CIRCLEHOME)/app/lib/zxsmi/libzxsmi.a \
          $(CIRCLEHOME)/app/lib/zxscreen/libzxscreen.a \
          $(CIRCLEHOME)/lib/sched/libsched.a \
          $(CIRCLEHOME)/lib/usb/libusb.a \
          $(CIRCLEHOME)/lib/input/libinput.a \
          $(CIRCLEHOME)/lib/fs/libfs.a \
          $(CIRCLEHOME)/lib/net/libnet.a \
          $(CIRCLEHOME)/addon/wlan/libwlan.a \
          $(CIRCLEHOME)/addon/wlan/hostap/wpa_supplicant/libwpa_supplicant.a \
          $(CIRCLEHOME)/addon/fatfs/libfatfs.a \
          $(CIRCLEHOME)/addon/SDCard/libsdcard.a \
          $(CIRCLEHOME)/lib/libcircle.a

include ../Rules.mk

-include $(DEPS)
