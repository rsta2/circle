#ifndef _linux_linuxdevice_h
#define _linux_linuxdevice_h

#include <linux/platform_device.h>
#include <circle/memory.h>
#include <circle/interrupt.h>
#include <circle/types.h>

typedef int TLinuxDeviceProbe (struct platform_device *pPlatformDevice);

class CLinuxDevice
{
public:
	CLinuxDevice (CMemorySystem	*pMemory,
		      CInterruptSystem	*pInterrupt,
		      TLinuxDeviceProbe	*pProbeFunc);
	~CLinuxDevice (void);

	boolean Initialize (void);

protected:
	void AddResource (u32 nStart, u32 nEnd, unsigned nFlags);
	void SetDMAMemory (u32 nStart, u32 nEnd);

private:
	struct platform_device m_PlatformDevice;

	TLinuxDeviceProbe *m_pProbeFunc;
};

#endif
