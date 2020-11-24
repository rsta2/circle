#
# Makefile
#

CIRCLEHOME = ../..

LVGL_DIR = .
LVGL_DIR_NAME = lvgl

CSRCS :=
include $(LVGL_DIR)/lvgl/lvgl.mk
OBJS := lvgl.o $(CSRCS:.c=.o)

CSRCS :=
-include $(LVGL_DIR)/lv_examples/lv_examples.mk
EXOBJS := $(CSRCS:.c=.o)

EXTRACLEAN += $(EXOBJS) $(EXOBJS:.o=.d)

all: liblvgl.a liblv_examples.a

liblvgl.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

liblv_examples.a: $(EXOBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(EXOBJS)

include $(CIRCLEHOME)/Rules.mk

INCLUDE += -I $(LVGL_DIR)

-include $(DEPS)

ifeq ($(strip $(CHECK_DEPS)),1)
-include $(EXOBJS:.o=.d)
endif
