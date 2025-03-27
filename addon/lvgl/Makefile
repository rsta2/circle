#
# Makefile
#

CIRCLEHOME = ../..

LVGL_PATH = ./lvgl

# From lvgl/lvgl.mk:
CSRCS += $(shell find $(LVGL_PATH)/src -type f -name *.c)
#CSRCS += $(shell find $(LVGL_PATH)/demos -type f -name *.c)
#CSRCS += $(shell find $(LVGL_PATH)/examples -type f -name *.c)
CSRCS += $(shell find $(LVGL_PATH)/demos/widgets -type f -name *.c)

OBJS = lvgl.o $(CSRCS:.c=.o)

EXTRACLEAN += $(CSRCS:.c=.o) $(CSRCS:.c=.d)

liblvgl.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

INCLUDE += -I $(LVGL_PATH)

-include $(DEPS)
