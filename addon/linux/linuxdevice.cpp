#include <linux/linuxdevice.h>
#include <linux/linuxemu.h>
#include <circle/util.h>
#include <assert.h>

CLinuxDevice::CLinuxDevice (CMemorySystem     *pMemory,
			    CInterruptSystem  *pInterrupt,
			    TLinuxDeviceProbe *pProbeFunc)
:	m_pProbeFunc (pProbeFunc)
{
	memset (&m_PlatformDevice, 0, sizeof m_PlatformDevice);

	m_PlatformDevice.num_resources = 0;

	m_PlatformDevice.dev.pMemorySystem = pMemory;
	m_PlatformDevice.dev.pInterruptSystem = pInterrupt;
}

CLinuxDevice::~CLinuxDevice (void)
{
	assert (0);
}

boolean CLinuxDevice::Initialize (void)
{
	if (linuxemu_init () != 0)
	{
		return FALSE;
	}

	assert (m_pProbeFunc != 0);
	return (*m_pProbeFunc) (&m_PlatformDevice) == 0 ? TRUE : FALSE;
}

void CLinuxDevice::AddResource (u32 nStart, u32 nEnd, unsigned nFlags)
{
	assert (nStart <= nEnd);
	assert (nFlags != 0);

	unsigned i = m_PlatformDevice.num_resources++;
	assert (i < PLATFORM_DEVICE_MAX_RESOURCES);

	m_PlatformDevice.resource[i].start = nStart;
	m_PlatformDevice.resource[i].end = nEnd;
	m_PlatformDevice.resource[i].flags = nFlags;
}

void CLinuxDevice::SetDMAMemory (u32 nStart, u32 nEnd)
{
	assert (nStart <= nEnd);

	m_PlatformDevice.dev.dma_mem.start = nStart;
	m_PlatformDevice.dev.dma_mem.end = nEnd;
	m_PlatformDevice.dev.dma_mem.flags = IORESOURCE_DMA;
}
