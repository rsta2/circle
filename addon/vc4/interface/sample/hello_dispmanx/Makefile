#
# Makefile
#

CIRCLEHOME = ../../../../..

OBJS	= main.o kernel.o dispmanx.o

LIBS	= $(CIRCLEHOME)/addon/vc4/interface/bcm_host/libbcm_host.a \
	  $(CIRCLEHOME)/addon/vc4/interface/vmcs_host/libvmcs_host.a \
	  $(CIRCLEHOME)/addon/vc4/interface/vcos/libvcos.a \
	  $(CIRCLEHOME)/addon/vc4/vchiq/libvchiq.a \
	  $(CIRCLEHOME)/addon/linux/liblinuxemu.a \
	  $(CIRCLEHOME)/lib/sched/libsched.a \
	  $(CIRCLEHOME)/lib/libcircle.a

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
