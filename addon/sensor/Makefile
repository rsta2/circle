#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= hcsr04.o mpu6050.o bmp180.o mcp300x.o ky040.o

libsensor.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
