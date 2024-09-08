#
# Makefile
#

CIRCLEHOME = ../..
LVGL_DIR = lvgl

ifeq (,$(wildcard $(LVGL_DIR)/lvgl.mk))
    $(error LVGL submodule not checked out!)
endif

include $(LVGL_DIR)/lvgl.mk

OBJS = lvgl.o $(CSRCS:.c=.o)

EXTRACLEAN += $(CSRCS:.c=.o) $(CSRCS:.c=.d)

liblvgl.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

INCLUDE += -I $(LVGL_DIR)

-include $(DEPS)
