#
# Makefile
#

CIRCLEHOME = ../..

LVGL_DIR = .
LVGL_DIR_NAME = lvgl

include $(LVGL_DIR)/lvgl/lvgl.mk

OBJS = lvgl.o $(CSRCS:.c=.o)

EXTRACLEAN += $(CSRCS:.c=.o) $(CSRCS:.c=.d)

liblvgl.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

INCLUDE += -I $(LVGL_DIR)

-include $(DEPS)
