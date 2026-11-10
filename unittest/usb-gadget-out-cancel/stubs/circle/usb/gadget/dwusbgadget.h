#pragma once
#include <circle/types.h>
#include <circle/usb/usb.h>
class CDWHCIRegister;
class CDWUSBGadgetEndpoint;
class CDWUSBGadget
{
      public:
	static constexpr unsigned NumberOfInEPs = 7;
	CDWUSBGadgetEndpoint *m_pEP[8] = {};
	void AssignEndpoint (unsigned ep, CDWUSBGadgetEndpoint *p)
	{
		m_pEP[ep] = p;
	}
	void RemoveEndpoint (unsigned ep)
	{
		m_pEP[ep] = nullptr;
	}
	boolean WaitForBit (CDWHCIRegister *, u32, boolean, unsigned, unsigned = 1000);
	boolean FlushTxFIFO (unsigned, unsigned = 1000);
	unsigned deconfigurations = 0;
	void SetDeviceAddress (u8)
	{
	}
	boolean SetConfiguration (u8 n)
	{
		if (!n)
		{
			++deconfigurations;
		}
		return true;
	}
	boolean SetInterface (u16, u16)
	{
		return true;
	}
	int GetInterface (u16)
	{
		return 0;
	}
	size_t descriptorSize = 18;
	const void *GetDescriptor (u16, u16, size_t *size)
	{
		static const u8 desc[274] = {18, 1};
		*size = descriptorSize;
		return desc;
	}
	int OnClassOrVendorRequest (const TSetupData *, u8 *)
	{
		return 0;
	}
};
