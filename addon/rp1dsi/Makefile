#
# Makefile
#

CIRCLEHOME = ../..

all: librp1dsi.a

include $(CIRCLEHOME)/Rules.mk

ifeq ($(strip $(RASPPI)),5)
OBJS	= rpitouchscreen.o attinyregulator.o rp1dsihostcontroller.o tc358762dsibridge.o \
	  drm_mipi_dsi.o
endif

CFLAGS += -Wno-narrowing

librp1dsi.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

-include $(DEPS)
