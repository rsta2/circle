#
# Makefile
#

CIRCLEHOME = ../../../..

OBJS	= vcos_abort.o vcos_backtrace.o vcos_generic_blockpool.o vcos_generic_event_flags.o \
	  vcos_generic_named_sem.o vcos_generic_reentrant_mtx.o vcos_generic_safe_string.o \
	  vcos_init.o vcos_logcat.o vcos_mem_from_malloc.o vcos_msgqueue.o vcos_pthreads.o

libvcos.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

ifeq ($(strip $(AARCH)),64)
$(error AARCH = 64 is not supported here)
endif

-include $(DEPS)
