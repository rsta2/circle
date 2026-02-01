#
# Makefile
#

CIRCLEHOME = ../..

LVGL_PATH = ./lvgl

# From lvgl/lvgl.mk:
#ASRCS += $(shell find $(LVGL_PATH)/src -type f -name *.S)
CSRCS += $(shell find $(LVGL_PATH)/src -type f -name *.c)
#CSRCS += $(shell find $(LVGL_PATH)/demos -type f -name *.c)
#CSRCS += $(shell find $(LVGL_PATH)/examples -type f -name *.c)
CSRCS += $(shell find $(LVGL_PATH)/demos/widgets -type f -name *.c)
#CXXSRCS += $(shell find $(LVGL_PATH)/src -type f -name *.cpp)

OBJS = lvgl.o $(ASRCS:.S=.o) $(CSRCS:.c=.o) $(CXXSRCS:.cpp=.o)

EXTRACLEAN += $(ASRCS:.S=.o) $(ASRCS:.S=.d) \
	      $(CSRCS:.c=.o) $(CSRCS:.c=.d) \
	      $(CXXSRCS:.cpp=.o) $(CXXSRCS:.cpp=.d)

liblvgl.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

INCLUDE += -I $(LVGL_PATH)

-include $(DEPS)
