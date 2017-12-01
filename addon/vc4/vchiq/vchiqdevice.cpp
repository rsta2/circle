//
// vchiqdevice.cpp
//
#include <vc4/vchiq/vchiqdevice.h>
#include <circle/bcm2835.h>
#include <circle/bcm2835int.h>
#include <circle/sysconfig.h>

extern "C" int vchiq_probe (struct platform_device *pdev);

CVCHIQDevice::CVCHIQDevice (CMemorySystem *pMemory, CInterruptSystem *pInterrupt)
:	CLinuxDevice (pMemory, pInterrupt, vchiq_probe)
{
	AddResource (ARM_VCHIQ_BASE, ARM_VCHIQ_END, IORESOURCE_MEM);
	AddResource (ARM_IRQ_ARM_DOORBELL_0, ARM_IRQ_ARM_DOORBELL_0, IORESOURCE_IRQ);

	SetDMAMemory (pMemory->GetCoherentPage (COHERENT_SLOT_VCHIQ_START),
		      pMemory->GetCoherentPage (COHERENT_SLOT_VCHIQ_END) + PAGE_SIZE-1);
}
