#
# Makefile
#

CIRCLEHOME = ../..

all: librp1dsi.a

include $(CIRCLEHOME)/Rules.mk

ifeq ($(strip $(RASPPI)),5)
OBJS	= rpitouchscreen.o rp1dsihostcontroller.o drm_mipi_dsi.o \
	  rpitouchscreen1.o attinyregulator.o tc358762dsibridge.o \
	  rpitouchscreen2.o rpipanel2regulator.o ili9881panel.o

endif

CFLAGS += -Wno-narrowing

librp1dsi.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

-include $(DEPS)
