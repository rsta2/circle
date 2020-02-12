#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= profiler.o gmon.o mcount.o profil.o arm-mcount.o glibc_compat.o

libprofile.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
