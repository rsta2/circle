#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= main.o \
          kernel.o \
          backgroundtask.o \
          tapetask.o \
          screenprocessortask.o \
          triangle.o

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
          $(CIRCLEHOME)/addon/vc4/interface/bcm_host/libbcm_host.a \
          $(CIRCLEHOME)/addon/vc4/interface/khronos/libkhrn_client.a \
          $(CIRCLEHOME)/addon/vc4/interface/vmcs_host/libvmcs_host.a \
          $(CIRCLEHOME)/addon/vc4/interface/vcos/libvcos.a \
          $(CIRCLEHOME)/addon/vc4/vchiq/libvchiq.a \
          $(CIRCLEHOME)/addon/linux/liblinuxemu.a \
          $(CIRCLEHOME)/lib/libcircle.a

IMAGES	= Djenne_128_128.h Gaudi_128_128.h Lucca_128_128.h

EXTRACLEAN = $(IMAGES)

include ../Rules.mk

%.h: %.raw
	@echo "  GEN   $@"
	@$(CIRCLEHOME)/tools/converttool -b $< > $@

triangle.o: $(IMAGES)

-include $(DEPS)
