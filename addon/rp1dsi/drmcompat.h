#ifndef _rp1dsi_drmcompat_h
#define _rp1dsi_drmcompat_h

#include <circle/types.h>

struct drm_display_mode
{
	int clock;              // kHz
	u16 hdisplay;
	u16 hsync_start;
	u16 hsync_end;
	u16 htotal;
	// u16 hskew;

	u16 vdisplay;
	u16 vsync_start;
	u16 vsync_end;
	u16 vtotal;
	// u16 vscan;

	u32 flags;
#define DRM_MODE_FLAG_NHSYNC	(1<<1)
#define DRM_MODE_FLAG_NVSYNC	(1<<3)
};

#endif
