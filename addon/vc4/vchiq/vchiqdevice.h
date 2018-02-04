//
// vchiqdevice.h
//
#ifndef _vc4_vchiq_vchiqdevice_h
#define _vc4_vchiq_vchiqdevice_h

#include <linux/linuxdevice.h>
#include <circle/memory.h>
#include <circle/interrupt.h>

class CVCHIQDevice : public CLinuxDevice
{
public:
	CVCHIQDevice (CMemorySystem *pMemory, CInterruptSystem *pInterrupt);
};

#endif
