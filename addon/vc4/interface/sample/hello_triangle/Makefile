#
# Makefile
#

CIRCLEHOME = ../../../../..

OBJS	= main.o kernel.o triangle.o

LIBS	= $(CIRCLEHOME)/addon/vc4/interface/bcm_host/libbcm_host.a \
	  $(CIRCLEHOME)/addon/vc4/interface/khronos/libkhrn_client.a \
	  $(CIRCLEHOME)/addon/vc4/interface/vmcs_host/libvmcs_host.a \
	  $(CIRCLEHOME)/addon/vc4/interface/vcos/libvcos.a \
	  $(CIRCLEHOME)/addon/vc4/vchiq/libvchiq.a \
	  $(CIRCLEHOME)/addon/linux/liblinuxemu.a \
	  $(CIRCLEHOME)/lib/sched/libsched.a \
	  $(CIRCLEHOME)/lib/libcircle.a

IMAGES	= Djenne_128_128.h Gaudi_128_128.h Lucca_128_128.h

EXTRACLEAN = $(IMAGES) converttool

include $(CIRCLEHOME)/Rules.mk

%.h: %.raw
	@echo "  GEN   $@"
	@./converttool -b $< > $@

triangle.o: $(IMAGES)

$(IMAGES): converttool

converttool: converttool.c
	@echo "  TOOL  $@"
	@gcc -o converttool converttool.c

-include $(DEPS)
