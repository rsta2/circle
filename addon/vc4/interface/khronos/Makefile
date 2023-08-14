#
# Makefile
#

CIRCLEHOME = ../../../..

KHRN_OBJS = common/khrn_client.o common/khrn_client_cache.o common/khrn_client_global_image_map.o \
	    common/khrn_client_pointermap.o common/khrn_client_vector.o common/khrn_int_hash.o \
	    common/khrn_int_image.o common/khrn_int_util.o common/khrn_options.o \
	    common/khrn_client_platform_linux.o common/khrn_client_rpc_linux.o

EGL_OBJS = egl/egl_client.o egl/egl_client_config.o egl/egl_client_context.o \
	   egl/egl_client_get_proc.o egl/egl_client_surface.o

EXT_OBJS = ext/egl_brcm_driver_monitor_client.o ext/egl_brcm_flush_client.o \
	   ext/egl_brcm_global_image_client.o ext/egl_brcm_perf_monitor_client.o \
	   ext/egl_khr_lock_surface_client.o ext/egl_khr_sync_client.o \
	   ext/ext_gl_debug_marker.o ext/gl_oes_draw_texture_client.o \
	   ext/gl_oes_egl_image_client.o ext/gl_oes_framebuffer_object.o \
	   ext/gl_oes_map_buffer.o ext/gl_oes_matrix_palette_client.o \
	   ext/gl_oes_query_matrix_client.o

GLXX_OBJS = glxx/glxx_client.o

VG_OBJS	= vg/vg_client.o vg/vg_int_mat3x3.o

OBJS = $(KHRN_OBJS) $(EGL_OBJS) $(EXT_OBJS) $(GLXX_OBJS) $(VG_OBJS)

libkhrn_client.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

EXTRACLEAN = common/*.o egl/*.o ext/*.o glxx/*.o vg/*.o \
	     common/*.d egl/*.d ext/*.d glxx/*.d vg/*.d

include $(CIRCLEHOME)/Rules.mk

CFLAGS += -Wno-strict-overflow -Wno-array-bounds
DEFINE += -DEGL_SERVER_DISPMANX

ifeq ($(strip $(AARCH)),64)
$(error AARCH = 64 is not supported khronos)
endif

ifeq ($(strip $(RASPPI)),4)
$(warning RASPPI = 4 is not supported in khronos)
endif

-include $(DEPS)
