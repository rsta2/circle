#ifndef _linux_dma_mapping_h
#define _linux_dma_mapping_h

#include <linux/device.h>
#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif

void *dmam_alloc_coherent (struct device *dev, size_t size, dma_addr_t *dma_handle, gfp_t gfp);

#ifdef __cplusplus
}
#endif

#endif
